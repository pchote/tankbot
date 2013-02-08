//*****************************************************************************
//  Webserver implemented using libwebsocket
//
//  Copyright: 2013 Paul Chote
//  Based on libwebsockets-test-server, Copyright 2010-2011 Andy Green
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <syslog.h>
#include "webserver.h"
#include "avr.h"

#include "include/json-c/json.h"
#include "include/libwebsockets.h"

#define LOCAL_RESOURCE_PATH "/usr/share/tankbotserver"
#define CLAMP(x, min, max) (((x) >= (max)) ? (max) : (((x) <= (min)) ? (min) : (x)))

extern struct avr *avr;

struct serveable {
    const char *urlpath;
    const char *mimetype;
};

static const struct serveable whitelist[] = {
    { "/test.html", "text/html" },
};

struct session {
    struct libwebsocket *wsi;
    size_t debug_messages_head;
};

// TODO: Merge this with the definition in protocol.h
#define DEBUG_MESSAGE_MAX_LENGTH 200
#define DEBUG_MESSAGE_BUFFER_SIZE 128

struct webserver
{
    struct libwebsocket_context *context;

    // New data available to send
    bool dirty;

    // Circular buffer of debug messages
    char *debug_messages[DEBUG_MESSAGE_BUFFER_SIZE];
    size_t debug_messages_head;
    pthread_mutex_t debug_messages_mutex;
};

static void dump_handshake_info(struct lws_tokens *lwst)
{
    static const char *token_names[WSI_TOKEN_COUNT] = {
        /*[WSI_TOKEN_GET_URI]        =*/ "GET URI",
        /*[WSI_TOKEN_HOST]        =*/ "Host",
        /*[WSI_TOKEN_CONNECTION]    =*/ "Connection",
        /*[WSI_TOKEN_KEY1]        =*/ "key 1",
        /*[WSI_TOKEN_KEY2]        =*/ "key 2",
        /*[WSI_TOKEN_PROTOCOL]        =*/ "Protocol",
        /*[WSI_TOKEN_UPGRADE]        =*/ "Upgrade",
        /*[WSI_TOKEN_ORIGIN]        =*/ "Origin",
        /*[WSI_TOKEN_DRAFT]        =*/ "Draft",
        /*[WSI_TOKEN_CHALLENGE]        =*/ "Challenge",

        /* new for 04 */
        /*[WSI_TOKEN_KEY]        =*/ "Key",
        /*[WSI_TOKEN_VERSION]        =*/ "Version",
        /*[WSI_TOKEN_SWORIGIN]        =*/ "Sworigin",

        /* new for 05 */
        /*[WSI_TOKEN_EXTENSIONS]    =*/ "Extensions",

        /* client receives these */
        /*[WSI_TOKEN_ACCEPT]        =*/ "Accept",
        /*[WSI_TOKEN_NONCE]        =*/ "Nonce",
        /*[WSI_TOKEN_HTTP]        =*/ "Http",
        /*[WSI_TOKEN_MUXURL]    =*/ "MuxURL",
    };

    for (int n = 0; n < WSI_TOKEN_COUNT; n++) {
        if (lwst[n].token == NULL)
            continue;

        fprintf(stderr, "    %s = %s\n", token_names[n], lwst[n].token);
    }
}

// Serve plain HTTP data
static int callback_http(struct libwebsocket_context *context,
                         struct libwebsocket *wsi,
                         enum libwebsocket_callback_reasons reason, void *user,
                         void *in, size_t len)
{
    char buf[256];

    switch (reason)
    {
    case LWS_CALLBACK_HTTP:
    {
        size_t n = 0;
        for (; n < (sizeof(whitelist) / sizeof(whitelist[0]) - 1); n++)
            if (in && strcmp((const char *)in, whitelist[n].urlpath) == 0)
                break;

        sprintf(buf, LOCAL_RESOURCE_PATH"%s", whitelist[n].urlpath);
        printf("serving: %s\n", buf);

        if (libwebsockets_serve_http_file(context, wsi, buf, whitelist[n].mimetype))
            lwsl_err("Failed to send HTTP file\n");

        // File is sent asynchronously
        break;
    }
    case LWS_CALLBACK_HTTP_FILE_COMPLETION:
        // Kill the connection after send completes
        return 1;
    default:
        break;
    }

    return 0;
}

static int callback_tankbot(struct libwebsocket_context *context,
                            struct libwebsocket *wsi,
                            enum libwebsocket_callback_reasons reason,
                            void *user, void *in, size_t len)
{
    struct session *session = user;
    struct webserver *webserver = libwebsocket_context_user(context);

    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
        session->debug_messages_head = webserver->debug_messages_head;
        break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
        // TODO: Reduce the scope of this lock
        pthread_mutex_lock(&webserver->debug_messages_mutex);
        while (session->debug_messages_head != webserver->debug_messages_head)
        {
            // JSON framing plus NULL add 30 bytes to the message length
            const size_t buf_start = LWS_SEND_BUFFER_PRE_PADDING;
            const size_t buf_length = LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING + 
                                      DEBUG_MESSAGE_MAX_LENGTH + 30;
            char buf[buf_length];
            char *data = &buf[buf_start];

            // Construct debug message packet
            json_object *obj = json_object_new_object();
            json_object_object_add(obj, "type", json_object_new_string("m"));
            json_object_object_add(obj, "value", json_object_new_string(webserver->debug_messages[session->debug_messages_head]));

            int n = snprintf(data, buf_length - buf_start, json_object_to_json_string(obj),
                webserver->debug_messages[session->debug_messages_head]);
            json_object_put(obj);

            n = libwebsocket_write(wsi, (unsigned char *)data, n, LWS_WRITE_TEXT);
            if (n < 0) {
                lwsl_err("ERROR %d writing to socket\n", n);
                return 1;
            }

            if (++session->debug_messages_head == DEBUG_MESSAGE_BUFFER_SIZE)
                session->debug_messages_head = 0;
        }
        pthread_mutex_unlock(&webserver->debug_messages_mutex);

        break;

    case LWS_CALLBACK_RECEIVE:
        printf("Got message: %s\n", (char *)in);

        enum json_tokener_error err;
    	json_object *obj = json_tokener_parse_verbose((char *)in, &err);

        if (err)
        {
            printf("JSON parse error: %s\n", json_tokener_error_desc(err));
            break;
        }

        json_object *type_obj;
        if (!json_object_object_get_ex(obj, "type", &type_obj))
        {
            printf("Invalid JSON message\n");
            break;
        }

        switch (json_object_get_string(type_obj)[0])
        {
            case 'S':
            {
                json_object *left_obj, *right_obj;
                if (!json_object_object_get_ex(obj, "left", &left_obj) ||
                    !json_object_object_get_ex(obj, "right", &right_obj))
                {
                    printf("Invalid JSON message\n");
                    break;
                }

                double left = CLAMP(json_object_get_double(left_obj), 0, 1);
                double right = CLAMP(json_object_get_double(right_obj), 0, 1);

                printf("Got speeds %f %f\n", left, right);
                avr_set_speed(avr, left, right);

                break;
            }
        }

        json_object_put(obj);

        break;

    case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
        dump_handshake_info((struct lws_tokens *)(long)user);
        break;

    default:
        break;
    }

    return 0;
}

static struct libwebsocket_protocols protocols[] = {
    {"http-only", callback_http, 0},
    {"tankbot", callback_tankbot, sizeof(struct session)},
    {NULL, NULL, 0}
};

struct webserver *webserver_create(int port)
{
    struct webserver *webserver = calloc(1, sizeof(struct webserver));
    if (!webserver)
        return NULL;

    int syslog_options = LOG_PID | LOG_PERROR;
    int debug_level = 7;

    pthread_mutex_init(&webserver->debug_messages_mutex, NULL);

    // Set websocket logging options
    setlogmask(LOG_UPTO (LOG_DEBUG));
    openlog("lwsts", syslog_options, LOG_DAEMON);
    lws_set_log_level(debug_level, lwsl_emit_syslog);

    webserver->context = libwebsocket_create_context(port, NULL, protocols,
                NULL, NULL, NULL, NULL, -1, -1, 0, webserver);
    if (!webserver->context)
    {
        lwsl_err("libwebsocket init failed\n");
        free(webserver);
        return NULL;
    }

    return webserver;
}

void webserver_free(struct webserver *webserver)
{
    libwebsocket_context_destroy(webserver->context);

    closelog();
    free(webserver);
}

int webserver_tick(struct webserver *webserver, int timeout_ms)
{
    if (webserver->dirty)
    {
        webserver->dirty = false;
        libwebsocket_callback_on_writable_all_protocol(&protocols[1]);
    }
    return libwebsocket_service(webserver->context, timeout_ms);
}

void webserver_send_debug(struct webserver *webserver, const char *message, size_t length)
{
    pthread_mutex_lock(&webserver->debug_messages_mutex);
    if (webserver->debug_messages[webserver->debug_messages_head])
        free(webserver->debug_messages[webserver->debug_messages_head]);

    webserver->debug_messages[webserver->debug_messages_head] = strndup(message, length);

    if (++webserver->debug_messages_head == DEBUG_MESSAGE_BUFFER_SIZE)
        webserver->debug_messages_head = 0;

    webserver->dirty = true;
    pthread_mutex_unlock(&webserver->debug_messages_mutex);
}
