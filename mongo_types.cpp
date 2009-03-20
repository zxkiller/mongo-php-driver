//mongo_types.cpp
/**
 *  Copyright 2009 10gen, Inc.
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <php.h>
#include <mongo/client/dbclient.h>
#include <string.h>
#include <sys/time.h>

#include "mongo.h"
#include "bson.h"

extern zend_class_entry *mongo_bindata_class;
extern zend_class_entry *mongo_code_class;
extern zend_class_entry *mongo_date_class;
extern zend_class_entry *mongo_id_class;
extern zend_class_entry *mongo_regex_class;

/* {{{ mongo_id___construct() 
 */
PHP_FUNCTION( mongo_id___construct ) {
  char *id;
  int id_len;
  mongo::OID oid;

  if (ZEND_NUM_ARGS() == 1) { 
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &id, &id_len) == FAILURE ) {
      zend_error( E_WARNING, "incorrect parameter types" );
      RETURN_FALSE;
    } else {
      add_property_stringl(getThis(), "id", id, id_len, 1);
    }
  }
  else {
    oid.init();
    std::string str = oid.str();
    char *cstr = (char*)str.c_str();
    add_property_stringl( getThis(), "id", cstr, strlen(cstr), 1 );
  }
}
/* }}} */


/* {{{ mongo_id___toString() 
 */
PHP_FUNCTION( mongo_id___toString ) {
  zval *zid = zend_read_property( mongo_id_class, getThis(), "id", 2, 0 TSRMLS_CC );
  RETURN_STRING(Z_STRVAL_P(zid), 1);
}
/* }}} */

zval* bson_to_zval_oid(const mongo::OID oid TSRMLS_DC) {
  zval *zoid;
  
  const unsigned char *data = oid.getData();
  char buf[25];
  char *n = buf;
  int i;
  for(i = 0; i < 12; i++) {
    sprintf(n, "%02x", *data++);
    n += 2;
  } 
  *(n) = '\0';

  MAKE_STD_ZVAL(zoid);
  object_init_ex(zoid, mongo_id_class);
  add_property_stringl( zoid, "id", buf, strlen( buf ), 1 );
  return zoid;
}

void zval_to_bson_oid(zval **data, mongo::OID *oid TSRMLS_DC) {
  zval *zid = zend_read_property( mongo_id_class, *data, "id", 2, 0 TSRMLS_CC );
  char *cid = Z_STRVAL_P( zid );
  std::string id = string( cid );
  oid->init( id );
}



/* {{{ MongoDate mongo_date___construct() 
 */
PHP_FUNCTION( mongo_date___construct ) {
  timeval time;
  long ltime;

  if (ZEND_NUM_ARGS() == 0) {
    gettimeofday(&time, NULL);
    add_property_long( getThis(), "sec", time.tv_sec );
    add_property_long( getThis(), "usec", time.tv_usec );
  }
  else if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ltime) == SUCCESS) {
    add_property_long( getThis(), "sec", ltime );
    add_property_long( getThis(), "usec", 0 );
  }
  else {
    zend_error( E_WARNING, "incorrect parameter types, expected: __construct( long )" );
    RETURN_FALSE;
  }
}
/* }}} */


/* {{{ mongo_date___toString() 
 */
PHP_FUNCTION( mongo_date___toString ) {
  zval *zsec = zend_read_property( mongo_date_class, getThis(), "sec", 3, 0 TSRMLS_CC );
  long sec = Z_LVAL_P( zsec );
  zval *zusec = zend_read_property( mongo_date_class, getThis(), "usec", 4, 0 TSRMLS_CC );
  long usec = Z_LVAL_P( zusec );
  double dusec = (double)usec/1000000;

  return_value->type = IS_STRING;
  return_value->value.str.len = spprintf(&(return_value->value.str.val), 0, "%.8f %ld", dusec, sec);
}
/* }}} */


zval* bson_to_zval_date(unsigned long long date TSRMLS_DC) {
  zval *zd;
    
  MAKE_STD_ZVAL(zd);
  object_init_ex(zd, mongo_date_class);

  long usec = (date % 1000) * 1000;
  long sec = date / 1000;

  add_property_long( zd, "sec", sec );
  add_property_long( zd, "usec", usec );
  return zd;
}

unsigned long long zval_to_bson_date(zval **data TSRMLS_DC) {
  zval *zsec = zend_read_property( mongo_date_class, *data, "sec", 3, 0 TSRMLS_CC );
  long sec = Z_LVAL_P( zsec );
  zval *zusec = zend_read_property( mongo_date_class, *data, "usec", 4, 0 TSRMLS_CC );
  long usec = Z_LVAL_P( zusec );
  return (unsigned long long)(sec * 1000) + (unsigned long long)(usec/1000);
}


/* {{{ mongo_bindata___construct(string) 
 */
PHP_FUNCTION(mongo_bindata___construct) {
  char *bin;
  int bin_len, type;

  int argc = ZEND_NUM_ARGS();
  if (argc == 1 &&
      zend_parse_parameters(argc TSRMLS_CC, "s", &bin, &bin_len) != FAILURE) {
    type = mongo::ByteArray;
  }
  else if (argc != 2 ||
           zend_parse_parameters(argc TSRMLS_CC, "sl", &bin, &bin_len, &type) == FAILURE) {
    zend_error( E_ERROR, "incorrect parameter types, expected __construct(string, long)" );
  }
  
  add_property_stringl( getThis(), "bin", bin, strlen(bin), 1 );
  add_property_long( getThis(), "length", bin_len);
  add_property_long( getThis(), "type", type);
}
/* }}} */


/* {{{ mongo_bindata___toString() 
 */
PHP_FUNCTION(mongo_bindata___toString) {
  RETURN_STRING( "<Mongo Binary Data>", 1 );
}
/* }}} */


zval* bson_to_zval_bin( char *bin, int len, int type TSRMLS_DC) {
  zval *zbin;
    
  MAKE_STD_ZVAL(zbin);
  object_init_ex(zbin, mongo_bindata_class);

  add_property_stringl( zbin, "bin", bin, len, 1 );
  add_property_long( zbin, "length", len);
  add_property_long( zbin, "type", type);
  return zbin;
}

int zval_to_bson_bin(zval **data, char **bin, int *len TSRMLS_DC) {
  zval *zbin = zend_read_property( mongo_bindata_class, *data, "bin", 3, 0 TSRMLS_CC );
  *(bin) = Z_STRVAL_P( zbin );
  zval *zlen = zend_read_property( mongo_bindata_class, *data, "length", 6, 0 TSRMLS_CC );
  *(len) = Z_LVAL_P( zlen );
  zval *ztype = zend_read_property( mongo_bindata_class, *data, "type", 4, 0 TSRMLS_CC );
  return (int)Z_LVAL_P( ztype );
}


/* {{{ mongo_regex___construct(string) 
 */
PHP_FUNCTION( mongo_regex___construct ) {
  char *re;
  int re_len;

  switch (ZEND_NUM_ARGS()) {
  case 0:
    add_property_stringl( getThis(), "regex", "", 0, 1 );
    add_property_stringl( getThis(), "flags", "", 0, 1 );
    break;
  case 1:
    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &re, &re_len) == FAILURE ) {
      zend_error( E_WARNING, "incorrect parameter types" );
      RETURN_FALSE;
    }
    string r = string(re);
    int splitter = r.find_last_of( "/" );

    string newre = r.substr(1, splitter - 1);
    string newopts = r.substr(splitter + 1);

    add_property_stringl( getThis(), "regex", (char*)newre.c_str(), newre.length(), 1 );
    add_property_stringl( getThis(), "flags", (char*)newopts.c_str(), newre.length(), 1 );
    break;
  }
}
/* }}} */


/* {{{ mongo_regex___toString() 
 */
PHP_FUNCTION( mongo_regex___toString ) {
  zval *zre = zend_read_property( mongo_regex_class, getThis(), "regex", 5, 0 TSRMLS_CC );
  char *re = Z_STRVAL_P( zre );
  zval *zopts = zend_read_property( mongo_regex_class, getThis(), "flags", 5, 0 TSRMLS_CC );
  char *opts = Z_STRVAL_P( zopts );

  int re_len = strlen(re);
  int opts_len = strlen(opts);
  char field_name[re_len+opts_len+3];

  sprintf( field_name, "/%s/%s", re, opts );
  RETURN_STRING( field_name, 1 );
}
/* }}} */


zval* bson_to_zval_regex( char *re, char *flags TSRMLS_DC) {
  zval *zre;
    
  MAKE_STD_ZVAL(zre);
  object_init_ex(zre, mongo_regex_class);

  add_property_stringl( zre, "regex", re, strlen(re), 1 );
  add_property_stringl( zre, "flags", flags, strlen(flags), 1 );
  return zre;
}

void zval_to_bson_regex(zval **data, char **re, char **flags TSRMLS_DC) {
  zval *zre = zend_read_property( mongo_regex_class, *data, "regex", 5, 0 TSRMLS_CC );
  *(re) = Z_STRVAL_P(zre);
  zval *zflags = zend_read_property( mongo_regex_class, *data, "flags", 5, 0 TSRMLS_CC );
  *(flags) = Z_STRVAL_P(zflags);
}


/* {{{ mongo_code___construct(string) 
 */
PHP_FUNCTION( mongo_code___construct ) {
  char *code;
  int code_len;
  zval *zcope;

  int argc = ZEND_NUM_ARGS();
  switch (argc) {
  case 1:
    if (zend_parse_parameters(argc TSRMLS_CC, "s", &code, &code_len) == FAILURE) {
      zend_error( E_ERROR, "incorrect parameter types, expected __construct(string[, array])" );
      RETURN_FALSE;
    }
    ALLOC_INIT_ZVAL(zcope);
    array_init(zcope);
    add_property_zval( getThis(), "scope", zcope );
    // get rid of extra ref
    zval_ptr_dtor(&zcope);
    break;
  case 2:
    if (zend_parse_parameters(argc TSRMLS_CC, "sa", &code, &code_len, &zcope) == FAILURE) {
      zend_error( E_ERROR, "incorrect parameter types, expected __construct(string[, array])" );
      RETURN_FALSE;
    }  
    add_property_zval( getThis(), "scope", zcope );  
    break;
  default:
    zend_error( E_WARNING, "expected 1 or 2 parameters, got %d parameters", argc );
    RETURN_FALSE;
  }

  add_property_stringl( getThis(), "code", code, code_len, 1 );
}
/* }}} */


/* {{{ mongo_code___toString() 
 */
PHP_FUNCTION( mongo_code___toString ) {
  zval *zode = zend_read_property( mongo_code_class, getThis(), "code", 4, 0 TSRMLS_CC );
  char *code = Z_STRVAL_P( zode );
  RETURN_STRING( code, 1 );
}
/* }}} */


zval* bson_to_zval_code(const char *code, zval *scope TSRMLS_DC) {
  zval *zptr;

  ALLOC_INIT_ZVAL(zptr);
  object_init_ex(zptr, mongo_code_class);

  add_property_stringl(zptr, "code", (char*)code, strlen(code), 1 );
  add_property_zval(zptr, "scope", scope);

  return zptr;
}


void zval_to_bson_code(zval **data, char **code, mongo::BSONObjBuilder *scope TSRMLS_DC) {
  zval *zcode = zend_read_property(mongo_code_class, *data, "code", 4, 0 TSRMLS_CC);
  *(code) = Z_STRVAL_P( zcode );

  zval *zscope = zend_read_property(mongo_code_class, *data, "scope", 5, 0 TSRMLS_CC);
  php_array_to_bson(scope, Z_ARRVAL_P(zscope) TSRMLS_CC);
}