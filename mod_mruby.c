/*
// -------------------------------------------------------------------
// mod_mruby
//      This is a mruby module for Apache HTTP Server.
//
//      By matsumoto_r (MATSUMOTO, Ryosuke) Sep 2012 in Japan
//          Academic Center for Computing and Media Studies, Kyoto University
//          email: matsumoto_r at net.ist.i.kyoto-u.ac.jp
//
// Date     2012/04/21
// Version  0.01
//
// change log
//  2012/04/21 1.00 matsumoto_r first release
// -------------------------------------------------------------------

// -------------------------------------------------------------------
// How To Compile
// [Use DSO]
// apxs -i -c -l cap mod_mruby.c
//
// <add to httpd.conf or conf.d/mruby.conf>
// LoadModule mruby_module   modules/mod_mruby.so
//
// -------------------------------------------------------------------

// -------------------------------------------------------------------
// How To Use
//
//  * Set mruby for handler phase
//      mrubyHandler /path/to/file.mrb
//
// -------------------------------------------------------------------
*/

#include "apr_strings.h"
#include "apr_md5.h"
#include "apr_file_info.h"
#include "ap_config.h"
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "mpm_common.h"
#include <unistd.h>
#include <mruby.h>
#include <mruby/proc.h>
#include <compile.h>


#define MODULE_NAME        "mod_mruby"
#define MODULE_VERSION     "0.01"
#define UNSET              -1
#define SET                1
#define ON                 1
#define OFF                0

typedef struct {

    const char *mruby_handler_file;

} mruby_config_t;

module AP_MODULE_DECLARE_DATA mruby_module;

static void *create_config(apr_pool_t *p, server_rec *s)
{
    mruby_config_t *conf = apr_palloc(p, sizeof (*conf));

    conf->mruby_handler_file = NULL;

    return conf;
}


static const char *set_mruby_handler(cmd_parms *cmd, void *mconfig, const char *arg)
{
    mruby_config_t *conf = (mruby_config_t *)mconfig;
    const char *err = ap_check_cmd_context(cmd, NOT_IN_FILES | NOT_IN_LIMIT);

    if (err != NULL)
        return err;

    conf->mruby_handler_file = apr_pstrdup(cmd->pool, arg);

    return NULL;
}


static int mruby_init(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s)
{
    void *data;
    const char *userdata_key = "mruby_init";

    apr_pool_userdata_get(&data, userdata_key, s->process->pool);

    if (!data)
        apr_pool_userdata_set((const void *)1, userdata_key, apr_pool_cleanup_null, s->process->pool);
    else                                              
        ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, NULL, MODULE_NAME "/" MODULE_VERSION " mechanism enabled");
    
    return OK;
}

static int mruby_handler(request_rec *r)
{

    mruby_config_t *conf = ap_get_module_config(r->server->module_config, &mruby_module);

    mrb_state *mrb = mrb_open();
    struct mrb_parser_state* p;
    int n;
    FILE *mrb_file;
    
    mrb_file = fopen(conf->mruby_handler_file, "r");
    p = mrb_parse_file(mrb, mrb_file);
    n = mrb_generate_code(mrb, p->tree);
    mrb_pool_close(p->pool);
    mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_nil_value());

    return OK;
}


static const command_rec mruby_cmds[] = {

    AP_INIT_TAKE1("mrubyHandler", set_mruby_handler, NULL, RSRC_CONF | ACCESS_CONF, "hook for handler phase"),
    {NULL}
};


static void register_hooks(apr_pool_t *p)
{
    ap_hook_post_config(mruby_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler(mruby_handler, NULL, NULL, APR_HOOK_REALLY_FIRST);
}


module AP_MODULE_DECLARE_DATA mruby_module = {
    STANDARD20_MODULE_STUFF,
    NULL,                      /* dir config creater */
    NULL,                      /* dir merger */
    create_config,             /* server config */
    NULL,                      /* merge server config */
    mruby_cmds,                /* command apr_table_t */
    register_hooks             /* register hooks */
};
