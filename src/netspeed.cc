/*
 * GeoIP C library binding for nodejs
 *
 * Licensed under the GNU LGPL 2.1 license
 */

#include "netspeed.h"
#include "global.h"

using namespace native;

NetSpeed::NetSpeed() : db(NULL) {};

NetSpeed::~NetSpeed() {
    if (db) {
        GeoIP_delete(db);
    }
};

Persistent<FunctionTemplate> NetSpeed::constructor_template;

void NetSpeed::Init(Handle<Object> exports) {
    Nan::HandleScope scope;

    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
    NanAssignPersistent(constructor_template, tpl);
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    tpl->SetClassName(Nan::New<String>("NetSpeed"));

    tpl->PrototypeTemplate()->Set(Nan::New<String>("lookupSync"),
            Nan::New<FunctionTemplate>(lookupSync)->GetFunction());
    exports->Set(Nan::New<String>("NetSpeed"), tpl->GetFunction());
}

NAN_METHOD(NetSpeed::New) {
    Nan::HandleScope scope;

    NetSpeed *n = new NetSpeed();

    String::Utf8Value file_str(args[0]->ToString());
    const char * file_cstr = ToCString(file_str);
    bool cache_on = args[1]->ToBoolean()->Value();

    n->db = GeoIP_open(file_cstr, cache_on ? GEOIP_MEMORY_CACHE : GEOIP_STANDARD);

    if (n->db) {
        n->db_edition = GeoIP_database_edition(n->db);
        if (n->db_edition == GEOIP_NETSPEED_EDITION ||
                n->db_edition == GEOIP_NETSPEED_EDITION_REV1) {
            n->Wrap(args.This());
            NanReturnValue(args.This());
        } else {
            GeoIP_delete(n->db);  // free()'s the reference & closes fd
            return NanThrowError("Error: Not valid netspeed database");
        }
    } else {
        return NanThrowError("Error: Cannot open database");
    }
}


NAN_METHOD(NetSpeed::lookupSync) {
    Nan::HandleScope scope;

    NetSpeed *n = ObjectWrap::Unwrap<NetSpeed>(args.This());

    Local<Value> data = Nan::New(NanNull());
    
    static NanUtf8String *host_cstr = new NanUtf8String(args[0]);
    uint32_t ipnum = _GeoIP_lookupaddress(**host_cstr);

    if (ipnum <= 0) {
        NanReturnValue(NanNull());
    }

    int netspeed = GeoIP_id_by_ipnum(n->db, ipnum);

    if (netspeed < 0) {
        NanReturnValue(NanNull());
    } else if (netspeed == GEOIP_UNKNOWN_SPEED) {
        data = Nan::New<String>("Unknown");
    } else if (netspeed == GEOIP_DIALUP_SPEED) {
        data = Nan::New<String>("Dialup");
    } else if (netspeed == GEOIP_CABLEDSL_SPEED) {
        data = Nan::New<String>("CableDSL");
    } else if (netspeed == GEOIP_CORPORATE_SPEED) {
        data = Nan::New<String>("Corporate");
    }

    NanReturnValue(data);
}
