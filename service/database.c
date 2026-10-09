/* Raff catalog and user library. All SQL is private to the bundled service;
 * external requests provide values bound to prepared statements, never SQL. */
#include "database.h"
#include "../third_party/sqlite/sqlite3.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
static sqlite3 *database;

static JSValue query(JSContext *c, JSValueConst self, int argc, JSValueConst *argv) {
  (void)self;
  if (!database || argc < 2) return JS_ThrowInternalError(c,"Database unavailable");
  size_t size=0; const char *sql=JS_ToCStringLen(c,&size,argv[0]);
  if (!sql || size>32768) { JS_FreeCString(c,sql); return JS_ThrowRangeError(c,"Query limit"); }
  sqlite3_stmt *stmt=NULL; const char *tail=NULL;
  int rc=sqlite3_prepare_v2(database,sql,(int)size,&stmt,&tail);
  while(tail && isspace((unsigned char)*tail))tail++;
  int trailing=tail && *tail;
  JS_FreeCString(c,sql);
  if(rc!=SQLITE_OK || !stmt || trailing) {sqlite3_finalize(stmt);return JS_ThrowInternalError(c,"Query prepare: %s",sqlite3_errmsg(database));}
  JSValue len=JS_GetPropertyStr(c,argv[1],"length");uint32_t count=0;JS_ToUint32(c,&count,len);JS_FreeValue(c,len);
  if(count>64 || (int)count!=sqlite3_bind_parameter_count(stmt)){sqlite3_finalize(stmt);return JS_ThrowTypeError(c,"Query parameter mismatch");}
  for(uint32_t n=0;n<count;n++){
    JSValue v=JS_GetPropertyUint32(c,argv[1],n);
    if(JS_IsNull(v)||JS_IsUndefined(v))rc=sqlite3_bind_null(stmt,n+1);
    else if(JS_IsBool(v))rc=sqlite3_bind_int(stmt,n+1,JS_ToBool(c,v));
    else if(JS_IsNumber(v)){double d;JS_ToFloat64(c,&d,v);rc=sqlite3_bind_double(stmt,n+1,d);}
    else{size_t bytes=0;const char *s=JS_ToCStringLen(c,&bytes,v);rc=s&&bytes<=1048576?sqlite3_bind_text(stmt,n+1,s,(int)bytes,SQLITE_TRANSIENT):SQLITE_TOOBIG;JS_FreeCString(c,s);}
    JS_FreeValue(c,v);if(rc!=SQLITE_OK){sqlite3_finalize(stmt);return JS_ThrowRangeError(c,"Query value limit");}
  }
  JSValue rows=JS_NewArray(c);int n=0;size_t budget=0;
  while((rc=sqlite3_step(stmt))==SQLITE_ROW){
    if(n>=512){JS_FreeValue(c,rows);sqlite3_finalize(stmt);return JS_ThrowRangeError(c,"Paginate queries to 512 rows");}
    JSValue row=JS_NewObject(c);
    for(int col=0;col<sqlite3_column_count(stmt);col++){
      JSValue value=JS_NULL;int type=sqlite3_column_type(stmt,col);
      if(type==SQLITE_INTEGER)value=JS_NewFloat64(c,(double)sqlite3_column_int64(stmt,col));
      else if(type==SQLITE_FLOAT)value=JS_NewFloat64(c,sqlite3_column_double(stmt,col));
      else if(type==SQLITE_TEXT){const char *s=(const char*)sqlite3_column_text(stmt,col);int bytes=sqlite3_column_bytes(stmt,col);budget+=bytes;if(budget>4194304){JS_FreeValue(c,row);JS_FreeValue(c,rows);sqlite3_finalize(stmt);return JS_ThrowRangeError(c,"Query response limit");}value=JS_NewStringLen(c,s,bytes);}
      JS_SetPropertyStr(c,row,sqlite3_column_name(stmt,col),value);
    }
    JS_SetPropertyUint32(c,rows,n++,row);
  }
  sqlite3_finalize(stmt);
  if(rc!=SQLITE_DONE){JS_FreeValue(c,rows);return JS_ThrowInternalError(c,"Query: %s",sqlite3_errmsg(database));}
  JSValue result=JS_NewObject(c);JS_SetPropertyStr(c,result,"rows",rows);JS_SetPropertyStr(c,result,"changes",JS_NewInt32(c,sqlite3_changes(database)));return result;
}

int raff_database_init(JSContext *c){
  int rc=sqlite3_open_v2("/data/raff/native-v5/user-library.sqlite",&database,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_URI,NULL);
  if(rc!=SQLITE_OK)return rc;
  sqlite3_busy_timeout(database,1000);
  sqlite3_limit(database,SQLITE_LIMIT_LENGTH,8*1024*1024);
  const char *schema=
    "PRAGMA journal_mode=DELETE;PRAGMA synchronous=FULL;PRAGMA cache_size=-4096;PRAGMA temp_store=MEMORY;"
    "BEGIN IMMEDIATE;"
    "CREATE TABLE IF NOT EXISTS migrations(version INTEGER PRIMARY KEY,applied_at TEXT NOT NULL);"
    "CREATE TABLE IF NOT EXISTS target_profiles(id TEXT PRIMARY KEY,name TEXT NOT NULL,kind TEXT NOT NULL,configuration TEXT NOT NULL,detected_at TEXT);"
    "CREATE TABLE IF NOT EXISTS installations(id TEXT PRIMARY KEY,game_id TEXT,release_id TEXT,target_id TEXT NOT NULL,path TEXT NOT NULL,platform TEXT,core_id TEXT,metadata TEXT NOT NULL,added_at TEXT NOT NULL,last_seen TEXT NOT NULL,UNIQUE(target_id,path));"
    "CREATE INDEX IF NOT EXISTS installations_game ON installations(game_id);"
    "CREATE TABLE IF NOT EXISTS favorites(game_id TEXT PRIMARY KEY,added_at TEXT NOT NULL);"
    "CREATE TABLE IF NOT EXISTS launch_history(id TEXT PRIMARY KEY,installation_id TEXT NOT NULL,target_id TEXT NOT NULL,core_id TEXT,core_version TEXT,configuration TEXT,started_at TEXT NOT NULL,status TEXT NOT NULL,log TEXT,playtime_seconds INTEGER);"
    "CREATE TABLE IF NOT EXISTS scan_jobs(id TEXT PRIMARY KEY,target_id TEXT NOT NULL,state TEXT NOT NULL,progress TEXT NOT NULL,created_at TEXT NOT NULL);"
    "CREATE TABLE IF NOT EXISTS user_sources(id TEXT PRIMARY KEY,configuration TEXT NOT NULL,authorization TEXT NOT NULL);"
    "CREATE TABLE IF NOT EXISTS metadata_cache(game_id TEXT PRIMARY KEY,value TEXT NOT NULL,updated_at TEXT NOT NULL,expires_at TEXT);"
    "CREATE TABLE IF NOT EXISTS view_state(id TEXT PRIMARY KEY,value TEXT NOT NULL);"
    "CREATE TABLE IF NOT EXISTS setup(key TEXT PRIMARY KEY,value TEXT NOT NULL);"
    "INSERT OR IGNORE INTO migrations VALUES(9,strftime('%Y-%m-%dT%H:%M:%SZ','now'));"
    "CREATE TABLE IF NOT EXISTS transfer_jobs(id TEXT PRIMARY KEY,state TEXT NOT NULL,value TEXT NOT NULL,updated_at INTEGER NOT NULL);"
    "INSERT OR IGNORE INTO migrations VALUES(10,strftime('%Y-%m-%dT%H:%M:%SZ','now'));"
    "PRAGMA user_version=10;COMMIT;"
    "ATTACH DATABASE 'file:/data/homebrew/PPSA99178/assets/library.sqlite?mode=ro' AS catalog;"
    "ATTACH DATABASE 'file:/data/homebrew/PPSA99178/assets/downloads.sqlite?mode=ro' AS downloads;";
  rc=sqlite3_exec(database,schema,NULL,NULL,NULL);if(rc!=SQLITE_OK)return rc;
  JSValue global=JS_GetGlobalObject(c);JS_SetPropertyStr(c,global,"nativeSQL",JS_NewCFunction(c,query,"nativeSQL",2));JS_FreeValue(c,global);return 0;
}
void raff_database_close(void){if(database)sqlite3_close(database);database=NULL;}
