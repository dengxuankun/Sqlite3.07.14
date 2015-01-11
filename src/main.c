/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Main file for the SQLite library.  The routines in this file
** implement the programmer interface to the library.  Routines in
** other files are for internal use by SQLite and should not be
** accessed by users of the library.
main.c函数主要是SQLit Library的大部分接口,这个文件中的程序执行这些程序接口进入library.
在其他的文件中的程序是被sqlite在内部应用，不应该被library的用户接受。

*/
//包含必要的头文件
#include "sqliteInt.h"

#ifdef SQLITE_ENABLE_FTS3
# include "fts3.h"
#endif
#ifdef SQLITE_ENABLE_RTREE
# include "rtree.h"
#endif
#ifdef SQLITE_ENABLE_ICU
# include "sqliteicu.h"
#endif

#ifndef SQLITE_AMALGAMATION
/* IMPLEMENTATION-OF: R-46656-45156 The sqlite3_version[] string constant
** contains the text of SQLITE_VERSION macro. 
*/
//sqlite3_version[]被赋值为字符串常量，包含SQLITE_VERSION macro。
const char sqlite3_version[] = SQLITE_VERSION;
#endif

/* IMPLEMENTATION-OF: R-53536-42575 The sqlite3_libversion() function returns
** a pointer to the to the sqlite3_version[] string constant. 
*/
//这个sqlite3_libversion()函数返回一个指针给 qlite3_version[] 字符串常量。

const char *sqlite3_libversion(void){ return sqlite3_version; }

/* IMPLEMENTATION-OF: R-63124-39300 The sqlite3_sourceid() function returns a
** pointer to a string constant whose value is the same as the
** SQLITE_SOURCE_ID C preprocessor macro. 
*/
//这个sqlite3_sourceid()函数返回一个指针给一个值是SQLITE_SOURCE_ID C的字符串常量
const char *sqlite3_sourceid(void){ return SQLITE_SOURCE_ID; }

/* IMPLEMENTATION-OF: R-35210-63508 The sqlite3_libversion_number() function
** returns an integer equal to SQLITE_VERSION_NUMBER.
*/
//这个函数返回一个整型值SQLITE_VERSION_NUMBER.
int sqlite3_libversion_number(void){ return SQLITE_VERSION_NUMBER; }

/* IMPLEMENTATION-OF: R-20790-14025 The sqlite3_threadsafe() function returns
** zero if and only if SQLite was compiled with mutexing code omitted due to
** the SQLITE_THREADSAFE compile-time option being set to 0.
*/
//如果sqlite是被互斥体编译，函数返回0，因为值SQLITE_THREADSAFE的编译时间·被设定为0.
int sqlite3_threadsafe(void){ return SQLITE_THREADSAFE; }

#if !defined(SQLITE_OMIT_TRACE) && defined(SQLITE_ENABLE_IOTRACE)
/*
** If the following function pointer is not NULL and if
** SQLITE_ENABLE_IOTRACE is enabled, then messages describing
** I/O active are written using this function.  These messages
** are intended for debugging activity only.
*/
//如果下面的函数不为空并且如果SQLITE_ENABLE_IOTRACE可用，i/o信息的描述活动可以被写入这个函数，
//这些信息的目的是为了调试。
void (*sqlite3IoTrace)(const char*, ...) = 0;
#endif

/*
** If the following global variable points to a string which is the
** name of a directory, then that directory will be used to store
** temporary files.
**
** See also the "PRAGMA temp_store_directory" SQL command.
*/
//如果下面的全局变量指向一个字符串（是一个目录的名字），这个目录会被存储在临时文件。
char *sqlite3_temp_directory = 0;

/*
** If the following global variable points to a string which is the
** name of a directory, then that directory will be used to store
** all database files specified with a relative pathname.
**
** See also the "PRAGMA data_store_directory" SQL command.
*/
//如果下面的全局变量指向一个字符串（是一个目录的名字），
//这个目录会被存储在所有的数据文件中（以一个相对的路径名）
char *sqlite3_data_directory = 0;

/*
** Initialize SQLite.  
**
** This routine must be called to initialize the memory allocation,
** VFS, and mutex subsystems prior to doing any serious work with
** SQLite.  But as long as you do not compile with SQLITE_OMIT_AUTOINIT
** this routine will be called automatically by key routines such as
** sqlite3_open().  
**
** This routine is a no-op except on its very first call for the process,
** or for the first call after a call to sqlite3_shutdown.
**
** The first thread to call this routine runs the initialization to
** completion.  If subsequent threads call this routine before the first
** thread has finished the initialization process, then the subsequent
** threads must block until the first thread finishes with the initialization.
**
** The first thread might call this routine recursively.  Recursive
** calls to this routine should not block, of course.  Otherwise the
** initialization process would never complete.
**
** Let X be the first thread to enter this routine.  Let Y be some other
** thread.  Then while the initial invocation of this routine by X is
** incomplete, it is required that:
**
**    *  Calls to this routine from Y must block until the outer-most
**       call by X completes.
**
**    *  Recursive calls to this routine from thread X return immediately
**       without blocking.
*/
/*这是个主要的互斥体。
初始化sqlite
这个函数是被用来进行初始化存储分配。
*/
int sqlite3_initialize(void){
  MUTEX_LOGIC( sqlite3_mutex *pMaster; )       /* The main static mutex */
  int rc;                                      /* Result code */

#ifdef SQLITE_OMIT_WSD
  rc = sqlite3_wsd_init(4096, 24);
  if( rc!=SQLITE_OK ){
    return rc;
  }
#endif

  /* If SQLite is already completely initialized, then this call
  ** to sqlite3_initialize() should be a no-op.  But the initialization
  ** must be complete.  So isInit must not be set until the very end
  ** of this routine.
  */
  //如果sqlite已经初始化，sqlite3_initialize()不被执行，但是初始化一定的完成。
  //所以isInit一直没设定知道每个函数都结束。
  if( sqlite3GlobalConfig.isInit ) return SQLITE_OK;

  /* Make sure the mutex subsystem is initialized.  If unable to 
  ** initialize the mutex subsystem, return early with the error.
  ** If the system is so sick that we are unable to allocate a mutex,
  ** there is not much SQLite is going to be able to do.
  **
  ** The mutex subsystem must take care of serializing its own
  ** initialization.
  */
  //要确保互斥子系统是被初始化的，如果没有初始化互斥子系统，返回错误，
  //如果这个系统太不正常，我们不能去分配互斥体，就没有sqlite去做
  //这个互斥子系统一定要进行自己的一系类初始化。
  rc = sqlite3MutexInit();
  if( rc ) return rc;

  /* Initialize the malloc() system and the recursive pInitMutex mutex.
  ** This operation is protected by the STATIC_MASTER mutex.  Note that
  ** MutexAlloc() is called for a static mutex prior to initializing the
  ** malloc subsystem - this implies that the allocation of a static
  ** mutex must not require support from the malloc subsystem.
  */
  //初始化malloc()函数和递归pInitMutex互斥体。这个操作是被the STATIC_MASTER mutex所保护。
  //标记MutexAlloc()，为了一个静态互斥体在互斥子系统之前初始化，这指出了静态互斥体的分配
  //一定不能得到互斥子系统的支持。
  MUTEX_LOGIC( pMaster = sqlite3MutexAlloc(SQLITE_MUTEX_STATIC_MASTER); )
  sqlite3_mutex_enter(pMaster);
  sqlite3GlobalConfig.isMutexInit = 1;
  if( !sqlite3GlobalConfig.isMallocInit ){
    rc = sqlite3MallocInit();
  }
  if( rc==SQLITE_OK ){
    sqlite3GlobalConfig.isMallocInit = 1;
    if( !sqlite3GlobalConfig.pInitMutex ){
      sqlite3GlobalConfig.pInitMutex =
           sqlite3MutexAlloc(SQLITE_MUTEX_RECURSIVE);
      if( sqlite3GlobalConfig.bCoreMutex && !sqlite3GlobalConfig.pInitMutex ){
        rc = SQLITE_NOMEM;
      }
    }
  }
  if( rc==SQLITE_OK ){
    sqlite3GlobalConfig.nRefInitMutex++;
  }
  sqlite3_mutex_leave(pMaster);

  /* If rc is not SQLITE_OK at this point, then either the malloc
  ** subsystem could not be initialized or the system failed to allocate
  ** the pInitMutex mutex. Return an error in either case.  */
  //如果rc不是值SQLITE_OK，返回rc。子系统就不会被初始化，系统分配就失败。
  //返回一个错误
  if( rc!=SQLITE_OK ){
    return rc;
  }

  /* Do the rest of the initialization under the recursive mutex so
  ** that we will be able to handle recursive calls into
  ** sqlite3_initialize().  The recursive calls normally come through
  ** sqlite3_os_init() when it invokes sqlite3_vfs_register(), but other
  ** recursive calls might also be possible.
  **
  ** IMPLEMENTATION-OF: R-00140-37445 SQLite automatically serializes calls
  ** to the xInit method, so the xInit method need not be threadsafe.
  **
  ** The following mutex is what serializes access to the appdef pcache xInit
  ** methods.  The sqlite3_pcache_methods.xInit() all is embedded in the
  ** call to sqlite3PcacheInitialize().
  在递归互斥体下进行剩余的初始化。目的是我们能够去处理递归进入sqlite3_initialize()
  当递归触发了sqlite3_vfs_register()时就会进入sqlite3_os_init()，但是其他的递归也有可能性。
  sqlite会自动的序列化调用入xInit方法，所以xInit方法不需要线程安全。
  下面的互斥体是
  */
  
  sqlite3_mutex_enter(sqlite3GlobalConfig.pInitMutex);
  if( sqlite3GlobalConfig.isInit==0 && sqlite3GlobalConfig.inProgress==0 ){
    FuncDefHash *pHash = &GLOBAL(FuncDefHash, sqlite3GlobalFunctions);
    sqlite3GlobalConfig.inProgress = 1;
    memset(pHash, 0, sizeof(sqlite3GlobalFunctions));
    sqlite3RegisterGlobalFunctions();
    if( sqlite3GlobalConfig.isPCacheInit==0 ){
      rc = sqlite3PcacheInitialize();
    }
    if( rc==SQLITE_OK ){
      sqlite3GlobalConfig.isPCacheInit = 1;
      rc = sqlite3OsInit();
    }
    if( rc==SQLITE_OK ){
      sqlite3PCacheBufferSetup( sqlite3GlobalConfig.pPage, 
          sqlite3GlobalConfig.szPage, sqlite3GlobalConfig.nPage);
      sqlite3GlobalConfig.isInit = 1;
    }
    sqlite3GlobalConfig.inProgress = 0;
  }
  sqlite3_mutex_leave(sqlite3GlobalConfig.pInitMutex);

  /* Go back under the static mutex and clean up the recursive
  ** mutex to prevent a resource leak.
  */
  //返回静态互斥体并且清除递归防止资源泄漏。
  sqlite3_mutex_enter(pMaster);
  sqlite3GlobalConfig.nRefInitMutex--;
  if( sqlite3GlobalConfig.nRefInitMutex<=0 ){
    assert( sqlite3GlobalConfig.nRefInitMutex==0 );
    sqlite3_mutex_free(sqlite3GlobalConfig.pInitMutex);
    sqlite3GlobalConfig.pInitMutex = 0;
  }
  sqlite3_mutex_leave(pMaster);

  /* The following is just a sanity check to make sure SQLite has
  ** been compiled correctly.  It is important to run this code, but
  ** we don't want to run it too often and soak up CPU cycles for no
  ** reason.  So we run it once during initialization.
  */
  //下面是检查确保sqlite已经被正确编译。这对运行程序很重要。但是我们不能去经常运行它
  //所以在初始化之后就运行。
#ifndef NDEBUG
#ifndef SQLITE_OMIT_FLOATING_POINT
  /* This section of code's only "output" is via assert() statements. */
  //这部分仅仅是输出是通过assert（）来声明。
  if ( rc==SQLITE_OK ){
    u64 x = (((u64)1)<<63)-1;
    double y;
    assert(sizeof(x)==8);
    assert(sizeof(x)==sizeof(y));
    memcpy(&y, &x, 8);
    assert( sqlite3IsNaN(y) );
  }
#endif
#endif

  /* Do extra initialization steps requested by the SQLITE_EXTRA_INIT
  ** compile-time option.
  */
  //SQLITE_EXTRA_INIT要求做一些额外的初始化步骤。
#ifdef SQLITE_EXTRA_INIT
  if( rc==SQLITE_OK && sqlite3GlobalConfig.isInit ){
    int SQLITE_EXTRA_INIT(const char*);
    rc = SQLITE_EXTRA_INIT(0);
  }
#endif

  return rc;
}

/*
** Undo the effects of sqlite3_initialize().  Must not be called while
** there are outstanding database connections or memory allocations or
** while any part of SQLite is otherwise in use in any thread.  This
** routine is not threadsafe.  But it is safe to invoke this routine
** on when SQLite is already shut down.  If SQLite is already shut down
** when this routine is invoked, then this routine is a harmless no-op.
*/
//消除了sqlite3_initialize()的影响。有一些外数据连接或者是存储分配，sqlite的任何一部分
//都可以用，否则就用在任何线程里。这个函数不是线程安全。但是它是安全的当sqlite准备关闭的
//时候。如果sqlite已经关闭这个函数就不能运行。
int sqlite3_shutdown(void){
  if( sqlite3GlobalConfig.isInit ){
#ifdef SQLITE_EXTRA_SHUTDOWN
    void SQLITE_EXTRA_SHUTDOWN(void);
    SQLITE_EXTRA_SHUTDOWN();
#endif
    sqlite3_os_end();
    sqlite3_reset_auto_extension();
    sqlite3GlobalConfig.isInit = 0;
  }
  if( sqlite3GlobalConfig.isPCacheInit ){
    sqlite3PcacheShutdown();
    sqlite3GlobalConfig.isPCacheInit = 0;
  }
  if( sqlite3GlobalConfig.isMallocInit ){
    sqlite3MallocEnd();
    sqlite3GlobalConfig.isMallocInit = 0;

#ifndef SQLITE_OMIT_SHUTDOWN_DIRECTORIES
    /* The heap subsystem has now been shutdown and these values are supposed
    ** to be NULL or point to memory that was obtained from sqlite3_malloc(),
    ** which would rely on that heap subsystem; therefore, make sure these
    ** values cannot refer to heap memory that was just invalidated when the
    ** heap subsystem was shutdown.  This is only done if the current call to
    ** this function resulted in the heap subsystem actually being shutdown.
    */
	//堆子系统已经关闭，这些值假设是null或者指向包括qlite3_malloc()的存储，
	//这主要是依赖堆子系统，因此，确保这些值不会放入无效的堆子系统，当它关闭的时候，
	//如果目前对于这个函数的调用引起在堆子系统，确实会关闭。
    sqlite3_data_directory = 0;
    sqlite3_temp_directory = 0;
#endif
  }
  if( sqlite3GlobalConfig.isMutexInit ){
    sqlite3MutexEnd();
    sqlite3GlobalConfig.isMutexInit = 0;
  }

  return SQLITE_OK;
}

/*
** This API allows applications to modify the global configuration of
** the SQLite library at run-time.
**
** This routine should only be called when there are no outstanding
** database connections or memory allocations.  This routine is not
** threadsafe.  Failure to heed these warnings can lead to unpredictable
** behavior.
*/
//这个api对于修改sqlite图书馆在运行的时候全局配置允许应用。
//这个程序可以被调用只有当有一些好的数据连接或者是内存分配。
//这个程序不是线程安全。留心这些可以引起不可预料的行为警告
int sqlite3_config(int op, ...){
  va_list ap;
  int rc = SQLITE_OK;

  /* sqlite3_config() shall return SQLITE_MISUSE if it is invoked while
  ** the SQLite library is in use. */
  //当sqlite图书馆被唤起是可以的
  if( sqlite3GlobalConfig.isInit ) return SQLITE_MISUSE_BKPT;

  va_start(ap, op);
  switch( op ){

    /* Mutex configuration options are only available in a threadsafe
    ** compile. 
    */
	//互斥配置选项仅仅是在线程安全编译的时候有效。
#if defined(SQLITE_THREADSAFE) && SQLITE_THREADSAFE>0
    case SQLITE_CONFIG_SINGLETHREAD: {
      /* Disable all mutexing */
	  //互斥都不可用
      sqlite3GlobalConfig.bCoreMutex = 0;
      sqlite3GlobalConfig.bFullMutex = 0;
      break;
    }
    case SQLITE_CONFIG_MULTITHREAD: {
      /* Disable mutexing of database connections */
      /* Enable mutexing of core data structures */
	  //禁用关于数据连接互斥，可以使用核心数据结构。
      sqlite3GlobalConfig.bCoreMutex = 1;
      sqlite3GlobalConfig.bFullMutex = 0;
      break;
    }
    case SQLITE_CONFIG_SERIALIZED: {
      /* Enable all mutexing */
	  //可以使用所有的互斥
      sqlite3GlobalConfig.bCoreMutex = 1;
      sqlite3GlobalConfig.bFullMutex = 1;
      break;
    }
    case SQLITE_CONFIG_MUTEX: {
      /* Specify an alternative mutex implementation */
	  //指定另一个互斥的实现
      sqlite3GlobalConfig.mutex = *va_arg(ap, sqlite3_mutex_methods*);
      break;
    }
    case SQLITE_CONFIG_GETMUTEX: {
      /* Retrieve the current mutex implementation */
	  //检索当前的互斥的实现
      *va_arg(ap, sqlite3_mutex_methods*) = sqlite3GlobalConfig.mutex;
      break;
    }
#endif


    case SQLITE_CONFIG_MALLOC: {
      /* Specify an alternative malloc implementation */
	  //指定一个替代的malloc实现
      sqlite3GlobalConfig.m = *va_arg(ap, sqlite3_mem_methods*);
      break;
    }
    case SQLITE_CONFIG_GETMALLOC: {
      /* Retrieve the current malloc() implementation */
	  //检索当前的malloc()实现
      if( sqlite3GlobalConfig.m.xMalloc==0 ) sqlite3MemSetDefault();
      *va_arg(ap, sqlite3_mem_methods*) = sqlite3GlobalConfig.m;
      break;
    }
    case SQLITE_CONFIG_MEMSTATUS: {
      /* Enable or disable the malloc status collection */
	  //启用或禁用malloc状态采集
      sqlite3GlobalConfig.bMemstat = va_arg(ap, int);
      break;
    }
    case SQLITE_CONFIG_SCRATCH: {
      /* Designate a buffer for scratch memory space */
	  //对于内存的空间要指定一个缓存
      sqlite3GlobalConfig.pScratch = va_arg(ap, void*);
      sqlite3GlobalConfig.szScratch = va_arg(ap, int);
      sqlite3GlobalConfig.nScratch = va_arg(ap, int);
      break;
    }
    case SQLITE_CONFIG_PAGECACHE: {
      /* Designate a buffer for page cache memory space */
	  //对于内存的空间要致命一个缓存
      sqlite3GlobalConfig.pPage = va_arg(ap, void*);
      sqlite3GlobalConfig.szPage = va_arg(ap, int);
      sqlite3GlobalConfig.nPage = va_arg(ap, int);
      break;
    }

    case SQLITE_CONFIG_PCACHE: {
      /* no-op */
	  //不操作
      break;
    }
    case SQLITE_CONFIG_GETPCACHE: {
      /* now an error */
	  //一个错误
      rc = SQLITE_ERROR;
      break;
    }

    case SQLITE_CONFIG_PCACHE2: {
      /* Specify an alternative page cache implementation */
	  //指定一个可选择的页缓存也实现
      sqlite3GlobalConfig.pcache2 = *va_arg(ap, sqlite3_pcache_methods2*);
      break;
    }
    case SQLITE_CONFIG_GETPCACHE2: {
      if( sqlite3GlobalConfig.pcache2.xInit==0 ){
        sqlite3PCacheSetDefault();
      }
      *va_arg(ap, sqlite3_pcache_methods2*) = sqlite3GlobalConfig.pcache2;
      break;
    }

#if defined(SQLITE_ENABLE_MEMSYS3) || defined(SQLITE_ENABLE_MEMSYS5)
    case SQLITE_CONFIG_HEAP: {
      /* Designate a buffer for heap memory space */
	  //对于堆的空间要指定一个缓存
      sqlite3GlobalConfig.pHeap = va_arg(ap, void*);
      sqlite3GlobalConfig.nHeap = va_arg(ap, int);
      sqlite3GlobalConfig.mnReq = va_arg(ap, int);

      if( sqlite3GlobalConfig.mnReq<1 ){
        sqlite3GlobalConfig.mnReq = 1;
      }else if( sqlite3GlobalConfig.mnReq>(1<<12) ){
        /* cap min request size at 2^12 */
		//左移2的12次方
        sqlite3GlobalConfig.mnReq = (1<<12);
      }

      if( sqlite3GlobalConfig.pHeap==0 ){
        /* If the heap pointer is NULL, then restore the malloc implementation
        ** back to NULL pointers too.  This will cause the malloc to go
        ** back to its default implementation when sqlite3_initialize() is
        ** run.
        */
		//如果堆指针为空，恢复malloc实现返回空指针，这将导致malloc去回到默认的实现
		//当sqlite3_initialize()运行的时候。
        memset(&sqlite3GlobalConfig.m, 0, sizeof(sqlite3GlobalConfig.m));
      }else{
        /* The heap pointer is not NULL, then install one of the
        ** mem5.c/mem3.c methods. If neither ENABLE_MEMSYS3 nor
        ** ENABLE_MEMSYS5 is defined, return an error.
        */
		//堆指针不是空的，用一个mem5.c/mem3.c方法，如果没有定义ENABLE_MEMSYS3和
        //ENABLE_MEMSYS5，返回一个错误
#ifdef SQLITE_ENABLE_MEMSYS3
        sqlite3GlobalConfig.m = *sqlite3MemGetMemsys3();
#endif
#ifdef SQLITE_ENABLE_MEMSYS5
        sqlite3GlobalConfig.m = *sqlite3MemGetMemsys5();
#endif
      }
      break;
    }
#endif

    case SQLITE_CONFIG_LOOKASIDE: {
      sqlite3GlobalConfig.szLookaside = va_arg(ap, int);
      sqlite3GlobalConfig.nLookaside = va_arg(ap, int);
      break;
    }
    
    /* Record a pointer to the logger funcction and its first argument.
    ** The default is NULL.  Logging is disabled if the function pointer is
    ** NULL.
    */
	//记录一个指针给日志函数并且他的第一个值。默认是空的。禁用日志记录，如果这个函数的指针式空类型的。
    case SQLITE_CONFIG_LOG: {
      /* MSVC is picky about pulling func ptrs from va lists.
      ** http://support.microsoft.com/kb/47961
      ** sqlite3GlobalConfig.xLog = va_arg(ap, void(*)(void*,int,const char*));
      */
	  
      typedef void(*LOGFUNC_t)(void*,int,const char*);
      sqlite3GlobalConfig.xLog = va_arg(ap, LOGFUNC_t);
      sqlite3GlobalConfig.pLogArg = va_arg(ap, void*);
      break;
    }

    case SQLITE_CONFIG_URI: {
      sqlite3GlobalConfig.bOpenUri = va_arg(ap, int);
      break;
    }

    default: {
      rc = SQLITE_ERROR;
      break;
    }
  }
  va_end(ap);
  return rc;
}

/*
** Set up the lookaside buffers for a database connection.
** Return SQLITE_OK on success.  
** If lookaside is already active, return SQLITE_BUSY.
**
** The sz parameter is the number of bytes in each lookaside slot.
** The cnt parameter is the number of slots.  If pStart is NULL the
** space for the lookaside memory is obtained from sqlite3_malloc().
** If pStart is not NULL then it is sz*cnt bytes of memory to use for
** the lookaside memory.
*/
//设置一个后贝缓冲区为了数据的连接、成功了返回SQLITE_OK
static int setupLookaside(sqlite3 *db, void *pBuf, int sz, int cnt){
  void *pStart;
  if( db->lookaside.nOut ){
    return SQLITE_BUSY;
  }
  /* Free any existing lookaside buffer for this handle before
  ** allocating a new one so we don't have to have space for 
  ** both at the same time.
  */
  //为了接下来的处理，再分配新的一个要释放所有存在的后备缓存，
  //所以我们不能在同一时刻拥有空间
  if( db->lookaside.bMalloced ){
    sqlite3_free(db->lookaside.pStart);
  }
  /* The size of a lookaside slot after ROUNDDOWN8 needs to be larger
  ** than a pointer to be useful.
  */
  //ROUNDDOWN8 需要比一个指针大的一个后备槽
  sz = ROUNDDOWN8(sz);  /* IMP: R-33038-09382 */
  if( sz<=(int)sizeof(LookasideSlot*) ) sz = 0;
  if( cnt<0 ) cnt = 0;
  if( sz==0 || cnt==0 ){
    sz = 0;
    pStart = 0;
  }else if( pBuf==0 ){
    sqlite3BeginBenignMalloc();
    pStart = sqlite3Malloc( sz*cnt );  /* IMP: R-61949-35727 */
    sqlite3EndBenignMalloc();
    if( pStart ) cnt = sqlite3MallocSize(pStart)/sz;
  }else{
    pStart = pBuf;
  }
  db->lookaside.pStart = pStart;
  db->lookaside.pFree = 0;
  db->lookaside.sz = (u16)sz;
  if( pStart ){
    int i;
    LookasideSlot *p;
    assert( sz > (int)sizeof(LookasideSlot*) );
    p = (LookasideSlot*)pStart;
    for(i=cnt-1; i>=0; i--){
      p->pNext = db->lookaside.pFree;
      db->lookaside.pFree = p;
      p = (LookasideSlot*)&((u8*)p)[sz];
    }
    db->lookaside.pEnd = p;
    db->lookaside.bEnabled = 1;
    db->lookaside.bMalloced = pBuf==0 ?1:0;
  }else{
    db->lookaside.pEnd = 0;
    db->lookaside.bEnabled = 0;
    db->lookaside.bMalloced = 0;
  }
  return SQLITE_OK;
}

/*
** Return the mutex associated with a database connection.
*/
//返回一个数据库连接相关的互斥
sqlite3_mutex *sqlite3_db_mutex(sqlite3 *db){
  return db->mutex;
}

/*
** Free up as much memory as we can from the given database
** connection.
*/
//尽可能从已给的数据连接中清理内存
int sqlite3_db_release_memory(sqlite3 *db){
  int i;
  sqlite3_mutex_enter(db->mutex);
  sqlite3BtreeEnterAll(db);
  for(i=0; i<db->nDb; i++){
    Btree *pBt = db->aDb[i].pBt;
    if( pBt ){
      Pager *pPager = sqlite3BtreePager(pBt);
      sqlite3PagerShrink(pPager);
    }
  }
  sqlite3BtreeLeaveAll(db);
  sqlite3_mutex_leave(db->mutex);
  return SQLITE_OK;
}

/*
** Configuration settings for an individual database connection
*/
//为私有数据连接配置设定
int sqlite3_db_config(sqlite3 *db, int op, ...){
  va_list ap;
  int rc;
  va_start(ap, op);
  switch( op ){
    case SQLITE_DBCONFIG_LOOKASIDE: {
      void *pBuf = va_arg(ap, void*); /* IMP: R-26835-10964 */
      int sz = va_arg(ap, int);       /* IMP: R-47871-25994 */
      int cnt = va_arg(ap, int);      /* IMP: R-04460-53386 */
      rc = setupLookaside(db, pBuf, sz, cnt);
      break;
    }
    default: {
      static const struct {
        int op;      /* The opcode */
        u32 mask;    /* Mask of the bit in sqlite3.flags to set/clear */
      } aFlagOp[] = {
        { SQLITE_DBCONFIG_ENABLE_FKEY,    SQLITE_ForeignKeys    },
        { SQLITE_DBCONFIG_ENABLE_TRIGGER, SQLITE_EnableTrigger  },
      };
      unsigned int i;
      rc = SQLITE_ERROR; /* IMP: R-42790-23372 */
      for(i=0; i<ArraySize(aFlagOp); i++){
        if( aFlagOp[i].op==op ){
          int onoff = va_arg(ap, int);
          int *pRes = va_arg(ap, int*);
          int oldFlags = db->flags;
          if( onoff>0 ){
            db->flags |= aFlagOp[i].mask;
          }else if( onoff==0 ){
            db->flags &= ~aFlagOp[i].mask;
          }
          if( oldFlags!=db->flags ){
            sqlite3ExpirePreparedStatements(db);
          }
          if( pRes ){
            *pRes = (db->flags & aFlagOp[i].mask)!=0;
          }
          rc = SQLITE_OK;
          break;
        }
      }
      break;
    }
  }
  va_end(ap);
  return rc;
}


/*
** Return true if the buffer z[0..n-1] contains all spaces.
*/
//如果缓存z[0..n-1]包含所有的空间，返回真值
static int allSpaces(const char *z, int n){
  while( n>0 && z[n-1]==' ' ){ n--; }
  return n==0;
}

/*
** This is the default collating function named "BINARY" which is always
** available.
**
** If the padFlag argument is not NULL then space padding at the end
** of strings is ignored.  This implements the RTRIM collation.
*/
//这是一个默认称为"BINARY"的总是可用的排序函数，
//如果padflag argument不是空值，空间的空隙在字符串最后被忽略。
//这实现了RTRTM的核对。
static int binCollFunc(
  void *padFlag,
  int nKey1, const void *pKey1,
  int nKey2, const void *pKey2
){
  int rc, n;
  n = nKey1<nKey2 ? nKey1 : nKey2;
  rc = memcmp(pKey1, pKey2, n);
  if( rc==0 ){
    if( padFlag
     && allSpaces(((char*)pKey1)+n, nKey1-n)
     && allSpaces(((char*)pKey2)+n, nKey2-n)
    ){
      /* Leave rc unchanged at 0 */
	  //留给如此一个不可改变的0值。
    }else{
      rc = nKey1 - nKey2;
    }
  }
  return rc;
}

/*
** Another built-in collating sequence: NOCASE. 
**
** This collating sequence is intended to be used for "case independant
** comparison". SQLite's knowledge of upper and lower case equivalents
** extends only to the 26 characters used in the English language.
**
** At the moment there is only a UTF-8 implementation.
*/
//其他内置的排序序列：
//这个排序序列目的是被独立比较应用，sqlite高低识积极扩大到26个特点，在英语语言中
//此时，这只有TF-8 的编码格式
static int nocaseCollatingFunc(
  void *NotUsed,
  int nKey1, const void *pKey1,
  int nKey2, const void *pKey2
){
  int r = sqlite3StrNICmp(
      (const char *)pKey1, (const char *)pKey2, (nKey1<nKey2)?nKey1:nKey2);
  UNUSED_PARAMETER(NotUsed);
  if( 0==r ){
    r = nKey1-nKey2;
  }
  return r;
}

/*
** Return the ROWID of the most recent insert
*/
//返回这个最近的插入
sqlite_int64 sqlite3_last_insert_rowid(sqlite3 *db){
  return db->lastRowid;
}

/*
** Return the number of changes in the most recent call to sqlite3_exec().
*/
//对于sqlite3_exec().在最近的调用中返回改变的数量
int sqlite3_changes(sqlite3 *db){
  return db->nChange;
}

/*
** Return the number of changes since the database handle was opened.
*/
//从数据库处理开始，返回改变的数量
int sqlite3_total_changes(sqlite3 *db){
  return db->nTotalChange;
}

/*
** Close all open savepoints. This function only manipulates fields of the
** database handle object, it does not close any savepoints that may be open
** at the b-tree/pager level.
*/
//关闭所有打开的存储指针，这个函数仅仅是创建一个数据处理项目文件。
//这不能够关闭任何可能开着的 b-tree/pager level存储指针。

void sqlite3CloseSavepoints(sqlite3 *db){
  while( db->pSavepoint ){
    Savepoint *pTmp = db->pSavepoint;
    db->pSavepoint = pTmp->pNext;
    sqlite3DbFree(db, pTmp);
  }
  db->nSavepoint = 0;
  db->nStatement = 0;
  db->isTransactionSavepoint = 0;
}

/*
** Invoke the destructor function associated with FuncDef p, if any. Except,
** if this is not the last copy of the function, do not invoke it. Multiple
** copies of a single function are created when create_function() is called
** with SQLITE_ANY as the encoding.
*/
//运行破坏函数。如果这不是最近复制的函数，不要唤醒他，多个复制这个函数
//将会被创造。当when create_function()被QLITE_ANY 调用的时候。
static void functionDestroy(sqlite3 *db, FuncDef *p){
  FuncDestructor *pDestructor = p->pDestructor;
  if( pDestructor ){
    pDestructor->nRef--;
    if( pDestructor->nRef==0 ){
      pDestructor->xDestroy(pDestructor->pUserData);
      sqlite3DbFree(db, pDestructor);
    }
  }
}

/*
** Disconnect all sqlite3_vtab objects that belong to database connection
** db. This is called when db is being closed.
*/
//断开属于数据连接的所有sqlite3_vtab 项目。当db被关闭的时候。
static void disconnectAllVtab(sqlite3 *db){
#ifndef SQLITE_OMIT_VIRTUALTABLE
  int i;
  sqlite3BtreeEnterAll(db);
  for(i=0; i<db->nDb; i++){
    Schema *pSchema = db->aDb[i].pSchema;
    if( db->aDb[i].pSchema ){
      HashElem *p;
      for(p=sqliteHashFirst(&pSchema->tblHash); p; p=sqliteHashNext(p)){
        Table *pTab = (Table *)sqliteHashData(p);
        if( IsVirtual(pTab) ) sqlite3VtabDisconnect(db, pTab);
      }
    }
  }
  sqlite3BtreeLeaveAll(db);
#else
  UNUSED_PARAMETER(db);
#endif
}

/*
** Return TRUE if database connection db has unfinalized prepared
** statements or unfinished sqlite3_backup objects.  
*/
//如火数据连接db没有准备陈述或者没完成sqlite3_backup 项目，返回真值。
static int connectionIsBusy(sqlite3 *db){
  int j;
  assert( sqlite3_mutex_held(db->mutex) );
  if( db->pVdbe ) return 1;
  for(j=0; j<db->nDb; j++){
    Btree *pBt = db->aDb[j].pBt;
    if( pBt && sqlite3BtreeIsInBackup(pBt) ) return 1;
  }
  return 0;
}

/*
** Close an existing SQLite database
*/
//关闭已经存在的数据库数据
static int sqlite3Close(sqlite3 *db, int forceZombie){
  if( !db ){
    return SQLITE_OK;
  }
  if( !sqlite3SafetyCheckSickOrOk(db) ){
    return SQLITE_MISUSE_BKPT;
  }
  sqlite3_mutex_enter(db->mutex);

  /* Force xDisconnect calls on all virtual tables */
  //在虚拟表中集中xDisconnect的调用
  disconnectAllVtab(db);

  /* If a transaction is open, the disconnectAllVtab() call above
  ** will not have called the xDisconnect() method on any virtual
  ** tables in the db->aVTrans[] array. The following sqlite3VtabRollback()
  ** call will do so. We need to do this before the check for active
  ** SQL statements below, as the v-table implementation may be storing
  ** some prepared statements internally.
  */
  //如果交易正在打开， disconnectAllVtab()调用就不会被xDisconnect()方法所调用，在db->aVTrans[]
  //的虚拟表中，接下来的qlite3VtabRollback()调用也是如此，在检查sql声明之前，我们需要这些
  // v-table implementation 可能被存储在一些内部声明中。
  sqlite3VtabRollback(db);

  /* Legacy behavior (sqlite3_close() behavior) is to return
  ** SQLITE_BUSY if the connection can not be closed immediately.
  */
  //遗传行为是会返回SQLITE_BUSY ，如果这个链接没有被立即关闭。
  if( !forceZombie && connectionIsBusy(db) ){
    sqlite3Error(db, SQLITE_BUSY, "unable to close due to unfinalized "
       "statements or unfinished backups");
    sqlite3_mutex_leave(db->mutex);
    return SQLITE_BUSY;
  }

  /* Convert the connection into a zombie and then close it.
  */
  //改变连接进入僵尸模式并且关闭它。
  db->magic = SQLITE_MAGIC_ZOMBIE;
  sqlite3LeaveMutexAndCloseZombie(db);
  return SQLITE_OK;
}

/*
** Two variations on the public interface for closing a database
** connection. The sqlite3_close() version returns SQLITE_BUSY and
** leaves the connection option if there are unfinalized prepared
** statements or unfinished sqlite3_backups.  The sqlite3_close_v2()
** version forces the connection to become a zombie if there are
** unclosed resources, and arranges for deallocation when the last
** prepare statement or sqlite3_backup closes.
*/
//对于关闭数据连接在公用接口上，有两种样式。sqlite3_close() version 返回 SQLITE_BUSY
//sqlite3_close()版本返回SQLITE_BUSY和离开连接选项，如果有未完成的声明或者sqlite3_backups
//sqlite3_close_v2()目的是把链接变成僵尸模式，如果有未关闭的资源，并且解除分配，当最后的准备声明或
//sqlite3_backup关闭。
int sqlite3_close(sqlite3 *db){ return sqlite3Close(db,0); }
int sqlite3_close_v2(sqlite3 *db){ return sqlite3Close(db,1); }


/*
** Close the mutex on database connection db.
**
** Furthermore, if database connection db is a zombie (meaning that there
** has been a prior call to sqlite3_close(db) or sqlite3_close_v2(db)) and
** every sqlite3_stmt has now been finalized and every sqlite3_backup has
** finished, then free all resources.
*/
//关闭在数据连接db上的互斥体
void sqlite3LeaveMutexAndCloseZombie(sqlite3 *db){
  HashElem *i;                    /* Hash table iterator */
  int j;

  /* If there are outstanding sqlite3_stmt or sqlite3_backup objects
  ** or if the connection has not yet been closed by sqlite3_close_v2(),
  ** then just leave the mutex and return.
  */
  //如果有外在的sqlite3_stmt 或者是sqlite3_backup 项目，如果这个链接至今还没被sqlite3_close_v2()
  //关闭，然后释放和回收。
  if( db->magic!=SQLITE_MAGIC_ZOMBIE || connectionIsBusy(db) ){
    sqlite3_mutex_leave(db->mutex);
    return;
  }

  /* If we reach this point, it means that the database connection has
  ** closed all sqlite3_stmt and sqlite3_backup objects and has been
  ** pased to sqlite3_close (meaning that it is a zombie).  Therefore,
  ** go ahead and free all resources.
  */
//如果我们获得这个指针，这意味着数据库连接已经关闭所有的sqlite3_stmt 和 sqlite3_backup项目
//意味着他是僵尸状态，因此，放在前面，释放所有的资源。 
  /* Free any outstanding Savepoint structures. */
  sqlite3CloseSavepoints(db);

  /* Close all database connections */
  //关闭所有的数据库连接
  for(j=0; j<db->nDb; j++){
    struct Db *pDb = &db->aDb[j];
    if( pDb->pBt ){
      sqlite3BtreeClose(pDb->pBt);
      pDb->pBt = 0;
      if( j!=1 ){
        pDb->pSchema = 0;
      }
    }
  }
  /* Clear the TEMP schema separately and last */
  //清除TEMP表
  if( db->aDb[1].pSchema ){
    sqlite3SchemaClear(db->aDb[1].pSchema);
  }
  sqlite3VtabUnlockList(db);

  /* Free up the array of auxiliary databases */
  //释放这组辅助数据库
  sqlite3CollapseDatabaseArray(db);
  assert( db->nDb<=2 );
  assert( db->aDb==db->aDbStatic );

  /* Tell the code in notify.c that the connection no longer holds any
  ** locks and does not require any further unlock-notify callbacks.
  */
  //区别notify.c的编码。这个链接没有任何锁也没有要求任何更远的解锁通知
  sqlite3ConnectionClosed(db);

  for(j=0; j<ArraySize(db->aFunc.a); j++){
    FuncDef *pNext, *pHash, *p;
    for(p=db->aFunc.a[j]; p; p=pHash){
      pHash = p->pHash;
      while( p ){
        functionDestroy(db, p);
        pNext = p->pNext;
        sqlite3DbFree(db, p);
        p = pNext;
      }
    }
  }
  for(i=sqliteHashFirst(&db->aCollSeq); i; i=sqliteHashNext(i)){
    CollSeq *pColl = (CollSeq *)sqliteHashData(i);
    /* Invoke any destructors registered for collation sequence user data. */
	//触发任何注册表序列用户数据
    for(j=0; j<3; j++){
      if( pColl[j].xDel ){
        pColl[j].xDel(pColl[j].pUser);
      }
    }
    sqlite3DbFree(db, pColl);
  }
  sqlite3HashClear(&db->aCollSeq);
#ifndef SQLITE_OMIT_VIRTUALTABLE
  for(i=sqliteHashFirst(&db->aModule); i; i=sqliteHashNext(i)){
    Module *pMod = (Module *)sqliteHashData(i);
    if( pMod->xDestroy ){
      pMod->xDestroy(pMod->pAux);
    }
    sqlite3DbFree(db, pMod);
  }
  sqlite3HashClear(&db->aModule);
#endif

  sqlite3Error(db, SQLITE_OK, 0); /* Deallocates any cached error strings. 释放任何错误的字符串缓存*/
  if( db->pErr ){
    sqlite3ValueFree(db->pErr);
  }
  sqlite3CloseExtensions(db);

  db->magic = SQLITE_MAGIC_ERROR;

  /* The temp-database schema is allocated differently from the other schema
  ** objects (using sqliteMalloc() directly, instead of sqlite3BtreeSchema()).
  ** So it needs to be freed here. Todo: Why not roll the temp schema into
  ** the same sqliteMalloc() as the one that allocates the database 
  ** structure?
  */
  //这个临时数据库数据表示来自其他表项目所分配，不是sqlite3BtreeSchema()
  //所以他需要被释放。为什么不滚动临时数据表进入sqliteMalloc()作为分配数据库结构。
  sqlite3DbFree(db, db->aDb[1].pSchema);
  sqlite3_mutex_leave(db->mutex);
  db->magic = SQLITE_MAGIC_CLOSED;
  sqlite3_mutex_free(db->mutex);
  assert( db->lookaside.nOut==0 );  /* Fails on a lookaside memory leak 释放后备内存*/
  if( db->lookaside.bMalloced ){
    sqlite3_free(db->lookaside.pStart);
  }
  sqlite3_free(db);
}

/*
** Rollback all database files.  If tripCode is not SQLITE_OK, then
** any open cursors are invalidated ("tripped" - as in "tripping a circuit
** breaker") and made to return tripCode if there are any further
** attempts to use that cursor.
*/
//回滚所有的数据库文件，如果tripCode不是SQLITE_OK，任何打开的游标都是可利用的。
//如果有任何更进一步企图用这个游标返回tripCode，
void sqlite3RollbackAll(sqlite3 *db, int tripCode){
  int i;
  int inTrans = 0;
  assert( sqlite3_mutex_held(db->mutex) );
  sqlite3BeginBenignMalloc();
  for(i=0; i<db->nDb; i++){
    Btree *p = db->aDb[i].pBt;
    if( p ){
      if( sqlite3BtreeIsInTrans(p) ){
        inTrans = 1;
      }
      sqlite3BtreeRollback(p, tripCode);
      db->aDb[i].inTrans = 0;
    }
  }
  sqlite3VtabRollback(db);
  sqlite3EndBenignMalloc();

  if( db->flags&SQLITE_InternChanges ){
    sqlite3ExpirePreparedStatements(db);
    sqlite3ResetAllSchemasOfConnection(db);
  }

  /* Any deferred constraint violations have now been resolved. */
  //何延迟约束违反已被解决
  db->nDeferredCons = 0;

  /* If one has been configured, invoke the rollback-hook callback */
  //如果已经被定义，调用回滚的回调函数
  if( db->xRollbackCallback && (inTrans || !db->autoCommit) ){
    db->xRollbackCallback(db->pRollbackArg);
  }
}


/*
** Return a static string that describes the kind of error specified in the
** argument.
*/
//返回一个静态字符串，用来描述这种指定的错误
const char *sqlite3ErrStr(int rc){    /*此函数主要作用:根据错位类型码返回对应的错误信息*/
  static const char* const aMsg[] = {
    /* SQLITE_OK          */ "返回OK，没有错误 |not an error",
    /* SQLITE_ERROR       */ "SQL逻辑错误或者数据库丢失 |SQL logic error or missing database",
    /* SQLITE_INTERNAL    */ 0,
    /* SQLITE_PERM        */ "访问权限被拒绝 |access permission denied",
    /* SQLITE_ABORT       */ "返回请求查询终止 |callback requested query abort",
    /* SQLITE_BUSY        */ "数据库处于锁定状态 |database is locked",
    /* SQLITE_LOCKED      */ "数据库表处于锁定状态 |database table is locked",
    /* SQLITE_NOMEM       */ "内存溢出 |out of memory",
    /* SQLITE_READONLY    */ "尝试修改只读数据库 |attempt to write a readonly database",
    /* SQLITE_INTERRUPT   */ "中断 |interrupted",
    /* SQLITE_IOERR       */ "disk I/O 错误 |disk I/O error",
    /* SQLITE_CORRUPT     */ "数据库硬盘镜像出现错误，说明SQLite的内部数据格式,已经损坏 |database disk image is malformed",
    /* SQLITE_NOTFOUND    */ "未定义操作 |unknown operation",
    /* SQLITE_FULL        */ "数据库或磁盘已满 |database or disk is full",
    /* SQLITE_CANTOPEN    */ "不能打开数据库文件 |unable to open database file",
    /* SQLITE_PROTOCOL    */ "封锁协议 |locking protocol",
    /* SQLITE_EMPTY       */ "表中无数据 |table contains no data",
    /* SQLITE_SCHEMA      */ "数据库模式已经改变 |database schema has changed",
    /* SQLITE_TOOBIG      */ "字符串或二进制对象过大 |string or blob too big",
    /* SQLITE_CONSTRAINT  */ "约束失败 |constraint failed",
    /* SQLITE_MISMATCH    */ "数据类型不匹配 |datatype mismatch",
    /* SQLITE_MISUSE      */ "库例程调用未按顺序，这个问题应该是多线程访问数据库 造成资源紧缺引起的 |library routine called out of sequence",
    /* SQLITE_NOLFS       */ "不支持大文件 |large file support is disabled",
    /* SQLITE_AUTH        */ "认证失败 |authorization denied",
    /* SQLITE_FORMAT      */ "附加数据库格式错误 |auxiliary database format error",
    /* SQLITE_RANGE       */ "下界或列数越界 |bind or column index out of range",
    /* SQLITE_NOTADB      */ "文件加密或文件不是一个数据库文件|",
  };	
  const char *zErr = "unknown error";					 /*定义常量zErr*/
  switch( rc ){											 /*rc为错误类型*/	
    case SQLITE_ABORT_ROLLBACK: {		
      zErr = "abort due to ROLLBACK";
      break;
    }
    default: {
      rc &= 0xff;
      if( ALWAYS(rc>=0) && rc<ArraySize(aMsg) && aMsg[rc]!=0 ){
        zErr = aMsg[rc];
      }
      break;
    }
  }
  return zErr;
}

/*
** This routine implements a busy callback that sleeps and tries  
** again until a timeout value is reached.  The timeout value is
** an integer number of milliseconds passed in as the first
** argument. |这个程序实现一个繁忙的回调，一直调用直到超时才进入睡眠。超时值是一个int型的毫秒数
*/
static int sqliteDefaultBusyCallback(
 void *ptr,               /* 数据库连接指针 */
 int count                /* 表处于忙状态的时间数 */
){
#if SQLITE_OS_WIN || (defined(HAVE_USLEEP) && HAVE_USLEEP)
  static const u8 delays[] =
     { 1, 2, 5, 10, 15, 20, 25, 25,  25,  50,  50, 100 };		/*常量，定义延迟时间*/
  static const u8 totals[] =									/*常量，定义总共延迟时间*/
     { 0, 1, 3,  8, 18, 33, 53, 78, 103, 128, 178, 228 };
# define NDELAY ArraySize(delays)
  sqlite3 *db = (sqlite3 *)ptr;									/*将类型为sqlite3的指针*db指向ptr*/
  int timeout = db->busyTimeout;								/*超时时间为sqlite3的忙超时间*/
  int delay, prior;												/*定义延迟时间和提前时间*/

  assert( count>=0 );											/*根据count的数值确定延迟和提前的时间*/
  if( count < NDELAY ){
    delay = delays[count];
    prior = totals[count];
  }else{
    delay = delays[NDELAY-1];
    prior = totals[NDELAY-1] + delay*(count-(NDELAY-1));
  }
  if( prior + delay > timeout ){								/*prior+delay值比timeout大时，采取的处理方法*/
    delay = timeout - prior;
    if( delay<=0 ) return 0;
  }
  sqlite3OsSleep(db->pVfs, delay*1000);							/*睡眠时间delay*1000*/		
  return 1;
#else
  sqlite3 *db = (sqlite3 *)ptr;
  int timeout = ((sqlite3 *)ptr)->busyTimeout;
  if( (count+1)*1000 > timeout ){
    return 0;
  }
  sqlite3OsSleep(db->pVfs, 1000000);
  return 1;
#endif
}

/*
** Invoke the given busy handler.									|调用给定的忙处理程序
**
** This routine is called when an operation failed with a lock.		|当锁定操作失败时调用这个程序
** If this routine returns non-zero, the lock is retried.  If it    |当程序返回非0时，重新尝试锁定
** returns 0, the operation aborts with an SQLITE_BUSY error.		|当返回为0时，操作终止，并报SQLITE_BUSY错误
*/
int sqlite3InvokeBusyHandler(BusyHandler *p){						/*定义函数，参数为BusyHandler类型，返回类型为int*/
  int rc;
  if( NEVER(p==0) || p->xFunc==0 || p->nBusy<0 ) return 0;			/*当参数p为0或p->xFunc为0或 p->nBusy<0时，返回0*/
  rc = p->xFunc(p->pArg, p->nBusy);									/*否则*/
  if( rc==0 ){														/*当rc==0时，将p->nBusy赋值为-1*/
    p->nBusy = -1;
  }else{															/*否则，p->nBusy自增1*/
    p->nBusy++;
  }
  return rc; 
}

/*
** This routine sets the busy callback for an Sqlite database to the |这个程序完成对给定参数的sqlite数据库回调方法设置忙回调
** given callback function with the given argument.
*/
int sqlite3_busy_handler(						/*定义无参的函数，返回类型为int*/
  sqlite3 *db,
  int (*xBusy)(void*,int),
  void *pArg
){		
  sqlite3_mutex_enter(db->mutex);				/*设置sqlite3对象*db的对应参数*/
  db->busyHandler.xFunc = xBusy;
  db->busyHandler.pArg = pArg;
  db->busyHandler.nBusy = 0;
  sqlite3_mutex_leave(db->mutex);
  return SQLITE_OK;								/*设置完成，返回SQLITE_OK标识码*/
}

#ifndef SQLITE_OMIT_PROGRESS_CALLBACK
/*
** This routine sets the progress callback for an Sqlite database to the		|这个程序完成对给定参数的sqlite数据库回调方法设置进一步回调
** given callback function with the given argument. The progress callback will	|这个进一步回调将调用每个n0ps操作码
** be invoked every nOps opcodes.
*/
void sqlite3_progress_handler(					/*定义无参的函数，返回类型为空*/
  sqlite3 *db,									/*对应的变量*/
  int nOps,											
  int (*xProgress)(void*), 
  void *pArg
){
  sqlite3_mutex_enter(db->mutex);				/*　线程安全：加锁保护 */
  if( nOps>0 ){
    db->xProgress = xProgress;
    db->nProgressOps = nOps;
    db->pProgressArg = pArg;
  }else{
    db->xProgress = 0;
    db->nProgressOps = 0;
    db->pProgressArg = 0;
  }
  sqlite3_mutex_leave(db->mutex);				/*处理完毕，解除封锁*/
}
#endif


/*
** This routine installs a default busy handler that waits for the	|这个程序安装一个默认的忙处理程序 ，在返回0之前等待规定的毫秒数
** specified number of milliseconds before returning 0.
*/
int sqlite3_busy_timeout(sqlite3 *db, int ms){							/*定义一个毫秒数，当未到达该毫秒数时，sqlite会sleep并重试当前操作如果超过ms毫秒，*/
  if( ms>0 ){															/* 仍然申请不到需要的锁，当前操作返回sqlite_BUSY*/
    db->busyTimeout = ms;
    sqlite3_busy_handler(db, sqliteDefaultBusyCallback, (void*)db);
  }else{																/*当ms<=0时，清除busy handle，申请不到锁直接返回*/
    sqlite3_busy_handler(db, 0, 0);
  }
  return SQLITE_OK;
}

/*
** Cause any pending operation to stop at its earliest opportunity.		|造成任何挂起的操作在其最早开始时停止
*/
void sqlite3_interrupt(sqlite3 *db){									/*中断*/
  db->u1.isInterrupted = 1;
}


/*
** This function is exactly the same as sqlite3_create_function(), except	|这个方法的功能与sqlite3_create_function()相同
** that it is designed to be called by internal code. The difference is		 |这个方法是为内部代码调用设计的
** that if a malloc() fails in sqlite3_create_function(), an error code		 |区别是sqlite3_create_function()的分配内存操作失败时，会返回错误码并清除内存分配错误标示
** is returned and the mallocFailed flag cleared. 
*/
int sqlite3CreateFunc(													  /*一个SQlite内部函数*/
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int enc,
  void *pUserData,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value **),
  void (*xStep)(sqlite3_context*,int,sqlite3_value **),
  void (*xFinal)(sqlite3_context*),
  FuncDestructor *pDestructor
){
  FuncDef *p;
  int nName;

  assert( sqlite3_mutex_held(db->mutex) );
  if( zFunctionName==0 ||
      (xFunc && (xFinal || xStep)) || 
      (!xFunc && (xFinal && !xStep)) ||
      (!xFunc && (!xFinal && xStep)) ||
      (nArg<-1 || nArg>SQLITE_MAX_FUNCTION_ARG) ||
      (255<(nName = sqlite3Strlen30( zFunctionName))) ){
    return SQLITE_MISUSE_BKPT;
  }
  
#ifndef SQLITE_OMIT_UTF16
  /* If SQLITE_UTF16 is specified as the encoding type, transform this	  |如果SQLITE_UTF16为一种编码格式，
  ** to one of SQLITE_UTF16LE or SQLITE_UTF16BE using the				  |将他转换为使用SQLITE_UTF16NATIVE macro的SQLITE_UTF16LE或者SQLITE_UTF16BE
  ** SQLITE_UTF16NATIVE macro. SQLITE_UTF16 is not used internally.		  |在内部并不使用SQLITE_UTF16，因此需要转换格式
  **
  ** If SQLITE_ANY is specified, add three versions of the function		  |如果是SQLITE_ANY，则在哈希表中添加三种版本的函数
  ** to the hash table.
  */
  if( enc==SQLITE_UTF16 ){												  /*如果编码格式为SQLITE_UTF16，将其格式转换为SQLITE_UTF16NATIVE*/
    enc = SQLITE_UTF16NATIVE;
  }else if( enc==SQLITE_ANY ){											  /*如果编码格式为SQLITE_ANY*/
    int rc;
    rc = sqlite3CreateFunc(db, zFunctionName, nArg, SQLITE_UTF8,
         pUserData, xFunc, xStep, xFinal, pDestructor);
    if( rc==SQLITE_OK ){
      rc = sqlite3CreateFunc(db, zFunctionName, nArg, SQLITE_UTF16LE,
          pUserData, xFunc, xStep, xFinal, pDestructor);
    }
    if( rc!=SQLITE_OK ){
      return rc;
    }
    enc = SQLITE_UTF16BE;
  }
#else
  enc = SQLITE_UTF8;												      /*当编码格式既不是SQLITE_UTF16也不是SQLITE_ANY时，指定编码格式为SQLITE_UTF8*/
#endif
  
  /* Check if an existing function is being overridden or deleted. If so, |检查一个已有的函数是否被重载或者被删除
  ** and there are active VMs, then return SQLITE_BUSY. If a function	  |若果是的话，并且有激活的虚拟机，返回SQLITE_BUSY
  ** is being overridden/deleted but there are no active VMs, allow the	  |如果只是被重载或删除但是没有激活的虚拟机，允许操作继续执行，但所有的预编译将不可用
  ** operation to continue but invalidate all precompiled statements.
  */
  p = sqlite3FindFunction(db, zFunctionName, nName, nArg, (u8)enc, 0);
  if( p && p->iPrefEnc==enc && p->nArg==nArg ){
    if( db->activeVdbeCnt ){
      sqlite3Error(db, SQLITE_BUSY, 
        "unable to delete/modify user-function due to active statements");
      assert( !db->mallocFailed );
      return SQLITE_BUSY;
    }else{
      sqlite3ExpirePreparedStatements(db);
    }
  }

  p = sqlite3FindFunction(db, zFunctionName, nName, nArg, (u8)enc, 1);
  assert(p || db->mallocFailed);
  if( !p ){
    return SQLITE_NOMEM;
  }

  /* If an older version of the function with a configured destructor is  |如果已经存在一个旧版本的析构函数被替换，则调用这里的析构函数
  ** being replaced invoke the destructor function here. */
  functionDestroy(db, p);

  if( pDestructor ){
    pDestructor->nRef++;
  }
  p->pDestructor = pDestructor;
  p->flags = 0;
  p->xFunc = xFunc;
  p->xStep = xStep;
  p->xFinalize = xFinal;
  p->pUserData = pUserData;
  p->nArg = (u16)nArg;
  return SQLITE_OK;
}

/*
** Create new user functions.									|创建新的用户函数
*/
int sqlite3_create_function(
  sqlite3 *db,
  const char *zFunc,
  int nArg,
  int enc,
  void *p,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value **),
  void (*xStep)(sqlite3_context*,int,sqlite3_value **),
  void (*xFinal)(sqlite3_context*)
){
  return sqlite3_create_function_v2(db, zFunc, nArg, enc, p, xFunc, xStep,
                                    xFinal, 0);
}

int sqlite3_create_function_v2(
  sqlite3 *db,
  const char *zFunc,
  int nArg,
  int enc,
  void *p,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value **),
  void (*xStep)(sqlite3_context*,int,sqlite3_value **),
  void (*xFinal)(sqlite3_context*),
  void (*xDestroy)(void *)
){
  int rc = SQLITE_ERROR;
  FuncDestructor *pArg = 0;
  sqlite3_mutex_enter(db->mutex);                                                    /*线程安全：加锁保护*/
  if( xDestroy ){
    pArg = (FuncDestructor *)sqlite3DbMallocZero(db, sizeof(FuncDestructor));
    if( !pArg ){
      xDestroy(p);
      goto out;
    }
    pArg->xDestroy = xDestroy;
    pArg->pUserData = p;
  }
  rc = sqlite3CreateFunc(db, zFunc, nArg, enc, p, xFunc, xStep, xFinal, pArg);
  if( pArg && pArg->nRef==0 ){
    assert( rc!=SQLITE_OK );
    xDestroy(p);
    sqlite3DbFree(db, pArg);
  }

 out:
  rc = sqlite3ApiExit(db, rc);
  sqlite3_mutex_leave(db->mutex);                                                   /*处理完毕，解除封锁*/
  return rc;
}

#ifndef SQLITE_OMIT_UTF16
int sqlite3_create_function16(
  sqlite3 *db,
  const void *zFunctionName,
  int nArg,
  int eTextRep,
  void *p,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
){
  int rc;
  char *zFunc8;
  sqlite3_mutex_enter(db->mutex);                                                    /*线程安全：加锁保护*/
  assert( !db->mallocFailed );
  zFunc8 = sqlite3Utf16to8(db, zFunctionName, -1, SQLITE_UTF16NATIVE);
  rc = sqlite3CreateFunc(db, zFunc8, nArg, eTextRep, p, xFunc, xStep, xFinal,0);
  sqlite3DbFree(db, zFunc8);
  rc = sqlite3ApiExit(db, rc);
  sqlite3_mutex_leave(db->mutex);                                                   /*处理完毕，解除封锁*/
  return rc;
}
#endif


/*
** Declare that a function has been overloaded by a virtual table.		  |对一个已经被虚拟表重载的方法
**
** If the function already exists as a regular global function, then      |如果这个函数已经作为一个一般全局函数存在，这个程序就是一个（空操作）no-op
** this routine is a no-op.  If the function does not exist, then create  |如果这个函数不存在，则创建一个总是抛出run-time错误的新函数
** a new one that always throws a run-time error.  
**
** When virtual tables intend to provide an overloaded function, they	  |当虚拟表想要提供一个重载函数时，需要调用这个程序段来确认全局函数的存在
** should call this routine to make sure the global function exists.      |
** A global function must exist in order for name resolution to work      |为了名称解析能正常工作，一个全局函数必须存在
** properly.
*/
int sqlite3_overload_function(										      /*定义一个重载方法*/
  sqlite3 *db,
  const char *zName,
  int nArg
){
  int nName = sqlite3Strlen30(zName);
  int rc = SQLITE_OK;
  sqlite3_mutex_enter(db->mutex);
  if( sqlite3FindFunction(db, zName, nName, nArg, SQLITE_UTF8, 0)==0 ){
    rc = sqlite3CreateFunc(db, zName, nArg, SQLITE_UTF8,
                           0, sqlite3InvalidFunction, 0, 0, 0);
  }
  rc = sqlite3ApiExit(db, rc);
  sqlite3_mutex_leave(db->mutex);
  return rc;
}

#ifndef SQLITE_OMIT_TRACE
/*
** Register a trace function.  The pArg from the previously registered trace   |注册一个跟踪函数，函数返回pArg
** is returned.  
**
** A NULL trace function means that no tracing is executes.  A non-NULL        |一个空NULL值的跟踪函数表示不存在跟踪
** trace is a pointer to a function that is invoked at the start of each       |一个非空(non-NULL)的跟踪函数是一个在每个SQL状态开始时被调用的指针
** SQL statement.
*/
void *sqlite3_trace(sqlite3 *db, void (*xTrace)(void*,const char*), void *pArg){    /*定义跟踪函数*/
  void *pOld;
  sqlite3_mutex_enter(db->mutex);
  pOld = db->pTraceArg;
  db->xTrace = xTrace;
  db->pTraceArg = pArg;
  sqlite3_mutex_leave(db->mutex);
  return pOld;
}
/*
** Register a profile function.  The pArg from the previously registered	   |定义一个轮廓函数，函数返回pArg
** profile function is returned.  
**
** A NULL profile function means that no profiling is executes.  A non-NULL	   |一个空NULL值的轮廓函数表示不存在轮廓
** profile is a pointer to a function that is invoked at the conclusion of	   |一个非空(non-NULL)的轮廓函数是在正在运行的SQL状态的结尾才被调用的函数
** each SQL statement that is run.
*/
void *sqlite3_profile(														   /*定义轮廓函数*/
  sqlite3 *db,
  void (*xProfile)(void*,const char*,sqlite_uint64),
  void *pArg
){
  void *pOld;
  sqlite3_mutex_enter(db->mutex);
  pOld = db->pProfileArg;
  db->xProfile = xProfile;
  db->pProfileArg = pArg;
  sqlite3_mutex_leave(db->mutex);
  return pOld;
}
#endif /* SQLITE_OMIT_TRACE */											       /*结束*/

/*
** Register a function to be invoked when a transaction commits.			   |当一个事物提交时注册这个函数
** If the invoked function returns non-zero, then the commit becomes a		   |如果调用函数返回非零，则事物回滚
** rollback.
*/
void *sqlite3_commit_hook(
  sqlite3 *db,              /* Attach the hook to this database */			   /*将hook附件到数据库*/
  int (*xCallback)(void*),  /* Function to invoke on each commit */            /*提交的每阶段都会调用的函数*/
  void *pArg                /* Argument to the function */					   /*函数声明*/
){
  void *pOld;
  sqlite3_mutex_enter(db->mutex);                                               /*线程安全：加锁保护*/
  pOld = db->pCommitArg;
  db->xCommitCallback = xCallback;
  db->pCommitArg = pArg;
  sqlite3_mutex_leave(db->mutex);                                              /*处理完毕，解除封锁*/
  return pOld;
}

/*
** Register a callback to be invoked each time a row is updated,			   |当某一行发生更新操作室，调用这个注册回调函数
** inserted or deleted using this database connection.						   |插入或分离数据库连接
*/
void *sqlite3_update_hook(
  sqlite3 *db,              /* Attach the hook to this database */			   /*将hook附加到数据库*/
  void (*xCallback)(void*,int,char const *,char const *,sqlite_int64),
  void *pArg                /* Argument to the function */                     /*函数声明*/
){
  void *pRet;
  sqlite3_mutex_enter(db->mutex);											   /*线程安全：加锁保护*/
  pRet = db->pUpdateArg;
  db->xUpdateCallback = xCallback;
  db->pUpdateArg = pArg;
  sqlite3_mutex_leave(db->mutex);											   /*处理完毕，解除封锁*/
  return pRet;
}

/*
** Register a callback to be invoked each time a transaction is rolled         |当处于连接数据库状态的事物发生回滚时调用这个回调注册程序
** back by this database connection.
*/
void *sqlite3_rollback_hook(
  sqlite3 *db,              /* Attach the hook to this database */			   /*将hook附加到数据库*/
  void (*xCallback)(void*), /* Callback function */                            /*回调函数*/
  void *pArg                /* Argument to the function */                     /*函数声明*/
){
  void *pRet;
  sqlite3_mutex_enter(db->mutex);                                              /*线程安全：加锁保护*/
  pRet = db->pRollbackArg;
  db->xRollbackCallback = xCallback;
  db->pRollbackArg = pArg;
  sqlite3_mutex_leave(db->mutex);                                              /*处理完毕，解除封锁*/
  return pRet;
}

#ifndef SQLITE_OMIT_WAL
/*
** The sqlite3_wal_hook() callback registered by sqlite3_wal_autocheckpoint(). |sqlite3_wal_hook()回调函数是由sqlite3_wal_autocheckpoint()注册的
** Invoke sqlite3_wal_checkpoint if the number of frames in the log file	   |当frames的数量比sqlite3大时，调用sqlite3_wal_checkpoint()函数
** is greater than sqlite3.pWalArg cast to an integer (the value configured by |将pWalArg强制转换为整型，pWalArg是由wal_autocheckpoint()这个函数确定的
** wal_autocheckpoint()).
*/ 
int sqlite3WalDefaultHook(
  void *pClientData,     /* Argument */	                                       /*声明*/
  sqlite3 *db,           /* Connection */                                      /*数据库连接*/
  const char *zDb,       /* Database */                                        /*数据库*/
  int nFrame             /* Size of WAL */                                     /*WAL的大小*/
){
  if( nFrame>=SQLITE_PTR_TO_INT(pClientData) ){
    sqlite3BeginBenignMalloc();                                                /*开始分配内存*/
    sqlite3_wal_checkpoint(db, zDb);                                           /*检查指针*/
    sqlite3EndBenignMalloc();                                                  /*分配内存结束*/
  }
  return SQLITE_OK;
}
#endif /* SQLITE_OMIT_WAL */

/*  
** Configure an sqlite3_wal_hook() callback to automatically checkpoint        |配置函数sqlite3_wal_hook(),如果日志文件中存在nFrame或跟多的nFrame时,
** a database after committing a transaction if there are nFrame or			   |在事物提交之后，自动检查数据库事物提交之后
** more frames in the log file. Passing zero or a negative value as the        |当nFrame是自动检查失败时传递0或者一个负数值  
** nFrame parameter disables automatic checkpoints entirely.
**
** The callback registered by this function replaces any existing callback	   |这个回调程序可以替代任何由sqlite3_wal_hook()注册的回调函数
** registered using sqlite3_wal_hook(). Likewise, registering a callback	   |同样地，使用sqlite3_wal_hook()注册的回调程序可以使这个程序的自动检测配置失效
** using sqlite3_wal_hook() disables the automatic checkpoint mechanism
** configured by this function.
*/
int sqlite3_wal_autocheckpoint(sqlite3 *db, int nFrame){					   /*定义自动检测函数*/
#ifdef SQLITE_OMIT_WAL
  UNUSED_PARAMETER(db);
  UNUSED_PARAMETER(nFrame);
#else
  if( nFrame>0 ){
    sqlite3_wal_hook(db, sqlite3WalDefaultHook, SQLITE_INT_TO_PTR(nFrame));
  }else{
    sqlite3_wal_hook(db, 0, 0);
  }
#endif
  return SQLITE_OK;
}

/*
** Register a callback to be invoked each time a transaction is written	       |每当有一个事物写到当前连接数据库的写头日志时，注册一个回调程序
** into the write-ahead-log by this database connection.
*/
void *sqlite3_wal_hook(
  sqlite3 *db,                    /* Attach the hook to this db handle */      /*将hook附加到触发器上*/
  int(*xCallback)(void *, sqlite3*, const char*, int),
  void *pArg                      /* First argument passed to xCallback() */   /*通过函数xCallback()的第一个声明*/
){
#ifndef SQLITE_OMIT_WAL
  void *pRet;
  sqlite3_mutex_enter(db->mutex);											   /*线程安全：加锁保护*/
  pRet = db->pWalArg;
  db->xWalCallback = xCallback;
  db->pWalArg = pArg;
  sqlite3_mutex_leave(db->mutex);                                              /*处理完毕，解除封锁*/
  return pRet;
#else
  return 0;
#endif
}

/*
** Checkpoint database zDb.                                                    /*检查数据库zDb*/
*/
int sqlite3_wal_checkpoint_v2(
  sqlite3 *db,                    /* Database handle */                        /*数据库触发器*/
  const char *zDb,                /* Name of attached database (or NULL) */    /*附加数据库的名称，可为空*/
  int eMode,                      /* SQLITE_CHECKPOINT_* value */              /*SQLITE_CHECKPOINT_的值*/
  int *pnLog,                     /* OUT: Size of WAL log in frames */         /*框架中日志文件的大小*/
  int *pnCkpt                     /* OUT: Total number of frames checkpointed *//*框架检查结点的总个数*/
){
#ifdef SQLITE_OMIT_WAL
  return SQLITE_OK;
#else
  int rc;                         /* Return code */                            /*返回编码*/
  int iDb = SQLITE_MAX_ATTACHED;  /* sqlite3.aDb[] index of db to checkpoint *//*在sqlite3.aDb[]中的索引位置，整型*/

  /* Initialize the output variables to -1 in case an error occurs. */         /*防止出错，初始化输出值默认为-1*/
  if( pnLog ) *pnLog = -1;
  if( pnCkpt ) *pnCkpt = -1;												   /*完成初始化*/						

  assert( SQLITE_CHECKPOINT_FULL>SQLITE_CHECKPOINT_PASSIVE );
  assert( SQLITE_CHECKPOINT_FULL<SQLITE_CHECKPOINT_RESTART );
  assert( SQLITE_CHECKPOINT_PASSIVE+2==SQLITE_CHECKPOINT_RESTART );
  if( eMode<SQLITE_CHECKPOINT_PASSIVE || eMode>SQLITE_CHECKPOINT_RESTART ){
    return SQLITE_MISUSE;
  }

  sqlite3_mutex_enter(db->mutex);											   /*线程安全：加锁保护*/
  if( zDb && zDb[0] ){
    iDb = sqlite3FindDbName(db, zDb);
  }
  if( iDb<0 ){
    rc = SQLITE_ERROR;
    sqlite3Error(db, SQLITE_ERROR, "unknown database: %s", zDb);			   /*索引小于0时，报错*/
  }else{
    rc = sqlite3Checkpoint(db, iDb, eMode, pnLog, pnCkpt);  
    sqlite3Error(db, rc, 0);                                                   
  }
  rc = sqlite3ApiExit(db, rc);
  sqlite3_mutex_leave(db->mutex);                                              /*处理完毕，解除封锁*/
  return rc;
#endif
}


/*
** Checkpoint database zDb. If zDb is NULL, or if the buffer zDb points        |检查数据库ZDb，如果Zdb为空或者Zdb的缓冲区包含长度为0的字符串
** to contains a zero-length string, all attached databases are				   |则所有附加的数据库都是检查点
** checkpointed.
*/
int sqlite3_wal_checkpoint(sqlite3 *db, const char *zDb){
  return sqlite3_wal_checkpoint_v2(db, zDb, SQLITE_CHECKPOINT_PASSIVE, 0, 0);
}

#ifndef SQLITE_OMIT_WAL
/*
** Run a checkpoint on database iDb. This is a no-op if database iDb is        |在数据库iDb上运行checkpoint，当数据库iDb当前没有在WAL中出现时，不做任何操作
** not currently open in WAL mode.
**
** If a transaction is open on the database being checkpointed, this           |如果的虎踞哭处于检查状态时开启了一个事物
** function returns SQLITE_LOCKED and a checkpoint is not attempted. If        |这个方法将返回数据库锁定(SQLITE_LOCKED)和尝试检查失败
** an error occurs while running the checkpoint, an SQLite error code is       |如果在运行checkpoint时发生错误，将会返回SQLite错误码(SQLITE_IOERR),都则返回SQLITE_OK
** returned (i.e. SQLITE_IOERR). Otherwise, SQLITE_OK.
**
** The mutex on database handle db should be held by the caller. The mutex     |访问数据库时应该获取数据库操作互斥信号量
** associated with the specific b-tree being checkpointed is taken by          |当checkpoint处于运行状态时，与b_tree检测相关的互斥量需要被获取
** this function while the checkpoint is running.
**
** If iDb is passed SQLITE_MAX_ATTACHED, then all attached databases are       |如果iDb传递了数据库最大附加值(SQLITE_MAX_ATTACHED),则所有的附加数据库时检查过的。
** checkpointed. If an error is encountered it is returned immediately -       |当有错误发生时，立即返回，不需做任何尝试去检查剩余的数据库
** no attempt is made to checkpoint any remaining databases.
**
** Parameter eMode is one of SQLITE_CHECKPOINT_PASSIVE, FULL or RESTART.       |eMode是SQLITE_CHECKPOINT_PASSIVE的一个参数，全部或者重新开始
*/
int sqlite3Checkpoint(sqlite3 *db, int iDb, int eMode, int *pnLog, int *pnCkpt){
  int rc = SQLITE_OK;             /* Return code */                            /*返回码*/
  int i;                          /* Used to iterate through attached dbs */   /*迭代的附加数据库*/
  int bBusy = 0;                  /* True if SQLITE_BUSY has been encountered *//*数据库忙，则赋值为真*/

  assert( sqlite3_mutex_held(db->mutex) );
  assert( !pnLog || *pnLog==-1 );
  assert( !pnCkpt || *pnCkpt==-1 );

  for(i=0; i<db->nDb && rc==SQLITE_OK; i++){
    if( i==iDb || iDb==SQLITE_MAX_ATTACHED ){
      rc = sqlite3BtreeCheckpoint(db->aDb[i].pBt, eMode, pnLog, pnCkpt);       /*调用函数/方法sqlite3BtreeCheckpoint*/
      pnLog = 0;
      pnCkpt = 0;
      if( rc==SQLITE_BUSY ){
        bBusy = 1;
        rc = SQLITE_OK;
      }
    }
  }

  return (rc==SQLITE_OK && bBusy) ? SQLITE_BUSY : rc;
}
#endif /* SQLITE_OMIT_WAL */

/*
** This function returns true if main-memory should be used instead of         |当使用主存而不是临时文件来保存临时页面和状态时，函数返回真
** a temporary file for transient pager files and statement journals.
** The value returned depends on the value of db->temp_store (runtime          |返回值依赖于参数db->temp和SQLITE_TEMP_STORE的编译时间。
** parameter) and the compile time value of SQLITE_TEMP_STORE. The
** following table describes the relationship between these two values		   |下面的表格描述了这两个值和函数返回值之间的关系
** and this functions return value.
**
**   SQLITE_TEMP_STORE     db->temp_store     Location of temporary database   
**   -----------------     --------------     ------------------------------
**   0                     any                file      (return 0)
**   1                     1                  file      (return 0)
**   1                     2                  memory    (return 1)
**   1                     0                  file      (return 0)
**   2                     1                  file      (return 0)
**   2                     2                  memory    (return 1)
**   2                     0                  memory    (return 1)
**   3                     any                memory    (return 1)
*/
int sqlite3TempInMemory(const sqlite3 *db){
#if SQLITE_TEMP_STORE==1
  return ( db->temp_store==2 );
#endif
#if SQLITE_TEMP_STORE==2
  return ( db->temp_store!=1 );
#endif
#if SQLITE_TEMP_STORE==3
  return 1;
#endif
#if SQLITE_TEMP_STORE<1 || SQLITE_TEMP_STORE>3
  return 0;
#endif
}

/*
** Return UTF-8 encoded English language explanation of the most recent        |返回最近发生错误的英语解释，编码合适为UTF-8格式
** error.
*/
const char *sqlite3_errmsg(sqlite3 *db){                                       /*定义数据库出错时返回的信息，类型为char*/
  const char *z;
  if( !db ){
    return sqlite3ErrStr(SQLITE_NOMEM);                                        /*数据库不存在*/
  }
  if( !sqlite3SafetyCheckSickOrOk(db) ){                                       /*安全检查*/
    return sqlite3ErrStr(SQLITE_MISUSE_BKPT);
  }
  sqlite3_mutex_enter(db->mutex);                                              /*线程安全：加锁保护*/
  if( db->mallocFailed ){
    z = sqlite3ErrStr(SQLITE_NOMEM);
  }else{
    z = (char*)sqlite3_value_text(db->pErr);
    assert( !db->mallocFailed );                                               /*分配内存失败*/
    if( z==0 ){
      z = sqlite3ErrStr(db->errCode);
    }
  }
  sqlite3_mutex_leave(db->mutex);                                              /*处理完毕，解除封锁*/
  return z;
}

#ifndef SQLITE_OMIT_UTF16
/*
** Return UTF-16 encoded English language explanation of the most recent       |返回最近发生错误的英语解释，编码合适为UTF-16格式  
** error.
*/
const void *sqlite3_errmsg16(sqlite3 *db){
  static const u16 outOfMem[] = {
    'o', 'u', 't', ' ', 'o', 'f', ' ', 'm', 'e', 'm', 'o', 'r', 'y', 0
  };
  static const u16 misuse[] = {
    'l', 'i', 'b', 'r', 'a', 'r', 'y', ' ', 
    'r', 'o', 'u', 't', 'i', 'n', 'e', ' ', 
    'c', 'a', 'l', 'l', 'e', 'd', ' ', 
    'o', 'u', 't', ' ', 
    'o', 'f', ' ', 
    's', 'e', 'q', 'u', 'e', 'n', 'c', 'e', 0
  };

  const void *z;
  if( !db ){
    return (void *)outOfMem;
  }
  if( !sqlite3SafetyCheckSickOrOk(db) ){
    return (void *)misuse;
  }
  sqlite3_mutex_enter(db->mutex);                                              /*线程安全：加锁保护*/
  if( db->mallocFailed ){
    z = (void *)outOfMem;
  }else{
    z = sqlite3_value_text16(db->pErr);
    if( z==0 ){
      sqlite3ValueSetStr(db->pErr, -1, sqlite3ErrStr(db->errCode),
           SQLITE_UTF8, SQLITE_STATIC);
      z = sqlite3_value_text16(db->pErr);
    }
    /* A malloc() may have failed within the call to sqlite3_value_text16()    |当调用上面的sqlite3_value_text16()函数时，可能会发生内存分配malloc()失败的情况
    ** above. If this is the case, then the db->mallocFailed flag needs to     |如果发生了这种情况，则在返回之前需要清理db->mallocFailed 标志；
    ** be cleared before returning. Do this directly, instead of via           |直接做这个操作，而不是通过sqlite3ApiExit()，以此避免设置数据库句柄
    ** sqlite3ApiExit(), to avoid setting the database handle error message.
    */
    db->mallocFailed = 0;
  }
  sqlite3_mutex_leave(db->mutex);                                              /*处理完毕，解除封锁*/
  return z;
}
#endif /* SQLITE_OMIT_UTF16 */

/*
** Return the most recent error code generated by an SQLite routine. If NULL is|返回由SQLite程序最近产生的错误代码
** passed to this function, we assume a malloc() failed during sqlite3_open(). |如果为NULL，则认为在执行sqlite3_open()时内存分配失败
*/
int sqlite3_errcode(sqlite3 *db){
  if( db && !sqlite3SafetyCheckSickOrOk(db) ){
    return SQLITE_MISUSE_BKPT;
  }
  if( !db || db->mallocFailed ){
    return SQLITE_NOMEM;
  }
  return db->errCode & db->errMask;
}
int sqlite3_extended_errcode(sqlite3 *db){
  if( db && !sqlite3SafetyCheckSickOrOk(db) ){
    return SQLITE_MISUSE_BKPT;
  }
  if( !db || db->mallocFailed ){
    return SQLITE_NOMEM;
  }
  return db->errCode;
}

/*
** Create a new collating function for database "db".  The name is zName       |为数据库db创建一个新的回收函数，名称为zName，编码格式为enc
** and the encoding is enc.
*/
static int createCollation(                                                    /*定义一个静态的函数*/
  sqlite3* db,
  const char *zName, 
  u8 enc,                                                                      /*编码格式utf-8*/
  void* pCtx,
  int(*xCompare)(void*,int,const void*,int,const void*),
  void(*xDel)(void*)
){
  CollSeq *pColl;
  int enc2;
  int nName = sqlite3Strlen30(zName); 
  
  assert( sqlite3_mutex_held(db->mutex) );

  /* If SQLITE_UTF16 is specified as the encoding type, transform this         |如果SQLITE_UTF16是指定的编码格式。通过SQLITE_UTF16NATIVE将其转换为SQLITE_UTF16LE或SQLITE_UTF16BE格式
  ** to one of SQLITE_UTF16LE or SQLITE_UTF16BE using the
  ** SQLITE_UTF16NATIVE macro. SQLITE_UTF16 is not used internally.            |SQLITE_UTF16并不在函数内部使用
  */
  enc2 = enc;
  testcase( enc2==SQLITE_UTF16 );                                              /*检测编码格式是否为SQLITE_UTF16*/
  testcase( enc2==SQLITE_UTF16_ALIGNED );                                      /*检测格式是否为SQLITE_UTF16_ALIGNED*/
  if( enc2==SQLITE_UTF16 || enc2==SQLITE_UTF16_ALIGNED ){
    enc2 = SQLITE_UTF16NATIVE;
  }
  if( enc2<SQLITE_UTF8 || enc2>SQLITE_UTF16BE ){
    return SQLITE_MISUSE_BKPT;
  }

  /* Check if this call is removing or replacing an existing collation         |检查这个调用是否已经被移出或者替代了一个已经存在的回收序列。 
  ** sequence. If so, and there are active VMs, return busy. If there          |如果是的话，并且没有激活的虚拟机，返回busy
  ** are no active VMs, invalidate any pre-compiled statements.                |若果没有激活的虚拟机，所有预编译不可用
  */
  pColl = sqlite3FindCollSeq(db, (u8)enc2, zName, 0);
  if( pColl && pColl->xCmp ){
    if( db->activeVdbeCnt ){
      sqlite3Error(db, SQLITE_BUSY, 
        "unable to delete/modify collation sequence due to active statements");
      return SQLITE_BUSY;
    }
    sqlite3ExpirePreparedStatements(db);

    /* If collation sequence pColl was created directly by a call to           |如果回收序列pColl是由sqlite3_create_collation直接创建调用的，而不是由synthCollSeq()产生的。
    ** sqlite3_create_collation, and not generated by synthCollSeq(),          
    ** then any copies made by synthCollSeq() need to be invalidated.          |则所有由synthCollSeq()产生的副本都不可用
    ** Also, collation destructor - CollSeq.xDel() - function may need         |同样，回收析构函数-CollSeq.xDel()可能需要被调用
    ** to be called.
    */ 
    if( (pColl->enc & ~SQLITE_UTF16_ALIGNED)==enc2 ){
      CollSeq *aColl = sqlite3HashFind(&db->aCollSeq, zName, nName);
      int j;
      for(j=0; j<3; j++){
        CollSeq *p = &aColl[j];
        if( p->enc==pColl->enc ){
          if( p->xDel ){
            p->xDel(p->pUser);
          }
          p->xCmp = 0;
        }
      }
    }
  }

  pColl = sqlite3FindCollSeq(db, (u8)enc2, zName, 1);
  if( pColl==0 ) return SQLITE_NOMEM;
  pColl->xCmp = xCompare;
  pColl->pUser = pCtx;
  pColl->xDel = xDel;
  pColl->enc = (u8)(enc2 | (enc & SQLITE_UTF16_ALIGNED));
  sqlite3Error(db, SQLITE_OK, 0);
  return SQLITE_OK;
}


/*
** This array defines hard upper bounds on limit values.  The                  |这些数列的定义是用来约束受限值的。
** initializer must be kept in sync with the SQLITE_LIMIT_*                    |初始化时必须与SQLITE_LIMIT_*保持同步
** #defines in sqlite3.h.                                                      |具体定义包含在头文件sqlite3.h中
*/
static const int aHardLimit[] = {
  SQLITE_MAX_LENGTH,                                                           /*最大长度*/
  SQLITE_MAX_SQL_LENGTH,                                                       /*SQL语句最大长度*/
  SQLITE_MAX_COLUMN,                                                           /*最多列数*/
  SQLITE_MAX_EXPR_DEPTH,
  SQLITE_MAX_COMPOUND_SELECT,
  SQLITE_MAX_VDBE_OP,
  SQLITE_MAX_FUNCTION_ARG,
  SQLITE_MAX_ATTACHED,
  SQLITE_MAX_LIKE_PATTERN_LENGTH,
  SQLITE_MAX_VARIABLE_NUMBER,
  SQLITE_MAX_TRIGGER_DEPTH,
};

/*
** Make sure the hard limits are set to reasonable values                      /*确保这些固定的限制值是合理的*/
*/
#if SQLITE_MAX_LENGTH<100                                                      /*最大长度小于100，给出警告信息SQLITE_MAX_LENGTH must be at least 100*/
# error SQLITE_MAX_LENGTH must be at least 100
#endif
#if SQLITE_MAX_SQL_LENGTH<100                                                  /*SQL长度最少为100*/                     
# error SQLITE_MAX_SQL_LENGTH must be at least 100
#endif
#if SQLITE_MAX_SQL_LENGTH>SQLITE_MAX_LENGTH
# error SQLITE_MAX_SQL_LENGTH must not be greater than SQLITE_MAX_LENGTH
#endif
#if SQLITE_MAX_COMPOUND_SELECT<2
# error SQLITE_MAX_COMPOUND_SELECT must be at least 2
#endif
#if SQLITE_MAX_VDBE_OP<40
# error SQLITE_MAX_VDBE_OP must be at least 40
#endif
#if SQLITE_MAX_FUNCTION_ARG<0 || SQLITE_MAX_FUNCTION_ARG>1000
# error SQLITE_MAX_FUNCTION_ARG must be between 0 and 1000
#endif
#if SQLITE_MAX_ATTACHED<0 || SQLITE_MAX_ATTACHED>62
# error SQLITE_MAX_ATTACHED must be between 0 and 62
#endif
#if SQLITE_MAX_LIKE_PATTERN_LENGTH<1
# error SQLITE_MAX_LIKE_PATTERN_LENGTH must be at least 1
#endif
#if SQLITE_MAX_COLUMN>32767
# error SQLITE_MAX_COLUMN must not exceed 32767
#endif
#if SQLITE_MAX_TRIGGER_DEPTH<1
# error SQLITE_MAX_TRIGGER_DEPTH must be at least 1
#endif


/*
** Change the value of a limit.  Report the old value.                         |改变限制值，报告旧的约束值
** If an invalid limit index is supplied, report -1.                           |如果一个有效的约束索引是支持的，报告为-1
** Make no changes but still report the old value if the                       |当新的约束值是负值时，不做任何改变且报告旧值
** new limit is negative.
**
** A new lower limit does not shrink existing constructs.                      |一个新的比较低的限制不会缩小原有的构造器
** It merely prevents new constructs that exceed the limit                     |它仅仅阻止新的构造器超过已经形成的限制
** from forming.
*/
int sqlite3_limit(sqlite3 *db, int limitId, int newLimit){                     /*定义限制函数*/
  int oldLimit;


  /* EVIDENCE-OF: R-30189-54097 For each limit category SQLITE_LIMIT_NAME      |对于每一个受限制的类别SQLITE_LIMIT_NAME
  ** there is a hard upper bound set at compile-time by a C preprocessor       |通过一个被C处理器宏调用SQLITE_MAX_NAME在其编译时间里确定一个固定的上限
  ** macro called SQLITE_MAX_NAME. (The "_LIMIT_" in the name is changed to    |命名中的“_LIMIT_”会被改为“_MAX_”
  ** "_MAX_".)
  */
  assert( aHardLimit[SQLITE_LIMIT_LENGTH]==SQLITE_MAX_LENGTH );
  assert( aHardLimit[SQLITE_LIMIT_SQL_LENGTH]==SQLITE_MAX_SQL_LENGTH );
  assert( aHardLimit[SQLITE_LIMIT_COLUMN]==SQLITE_MAX_COLUMN );
  assert( aHardLimit[SQLITE_LIMIT_EXPR_DEPTH]==SQLITE_MAX_EXPR_DEPTH );
  assert( aHardLimit[SQLITE_LIMIT_COMPOUND_SELECT]==SQLITE_MAX_COMPOUND_SELECT);
  assert( aHardLimit[SQLITE_LIMIT_VDBE_OP]==SQLITE_MAX_VDBE_OP );
  assert( aHardLimit[SQLITE_LIMIT_FUNCTION_ARG]==SQLITE_MAX_FUNCTION_ARG );
  assert( aHardLimit[SQLITE_LIMIT_ATTACHED]==SQLITE_MAX_ATTACHED );
  assert( aHardLimit[SQLITE_LIMIT_LIKE_PATTERN_LENGTH]==
                                               SQLITE_MAX_LIKE_PATTERN_LENGTH );
  assert( aHardLimit[SQLITE_LIMIT_VARIABLE_NUMBER]==SQLITE_MAX_VARIABLE_NUMBER);
  assert( aHardLimit[SQLITE_LIMIT_TRIGGER_DEPTH]==SQLITE_MAX_TRIGGER_DEPTH );
  assert( SQLITE_LIMIT_TRIGGER_DEPTH==(SQLITE_N_LIMIT-1) );


  if( limitId<0 || limitId>=SQLITE_N_LIMIT ){
    return -1;
  }
  oldLimit = db->aLimit[limitId];
  if( newLimit>=0 ){                   /* IMP: R-52476-28732 */
    if( newLimit>aHardLimit[limitId] ){
      newLimit = aHardLimit[limitId];  /* IMP: R-51463-25634 */
    }
    db->aLimit[limitId] = newLimit;
  }
  return oldLimit;                     /* IMP: R-53341-35419 */
}

/*
** This function is used to parse both URIs and non-URI filenames passed by the     |不管是通过sqlite3_open()的接口产生的的URIs还是sqlite3_open_v2()产生的非URI都可以用这个函数来解析
** user to API functions sqlite3_open() or sqlite3_open_v2(), and for database
** URIs specified as part of ATTACH statements.                                     |且在数据库中URIs被当做附加声明的一部分
**
** The first argument to this function is the name of the VFS to use (or            |如果URI不包含"vfs=xxx"的查询参数对这个方法的第一个声明是VFS的命名（或默认VFS的NULL值）
** a NULL to signify the default VFS) if the URI does not contain a "vfs=xxx"       |
** query parameter. The second argument contains the URI (or non-URI filename)      |第二个声明包含URI或非URI文件名
** itself. When this function is called the *pFlags variable should contain         |当这个方法调用*pFlags时应该包含默认的标记来打开数据库句柄
** the default flags to open the database handle with. The value stored in          |如果URI文件名中包含"cache=xxx" 或 "mode=xxx"查询参数时， 则在*pFlags中保存的值在返回时可能需要更新                
** *pFlags may be updated before returning if the URI filename contains 
** "cache=xxx" or "mode=xxx" query parameters.
**
** If successful, SQLITE_OK is returned. In this case *ppVfs is set to point to     |如果成功，返回SQLITE_OK
** the VFS that should be used to open the database file. *pzFile is set to         |在这里*ppVfs用来指向应该被用来打开数据库文件的的VFS
** point to a buffer containing the name of the file to open. It is the             |*pzFile用来指向包含打开文件名的缓冲区
** responsibility of the caller to eventually call sqlite3_free() to release        |最后应该调用sqlite3_free() 函数来清除这个缓冲区
** this buffer.
**
** If an error occurs, then an SQLite error code is returned and *pzErrMsg          |如果发生错误
** may be set to point to a buffer containing an English language error             |返回SQLite错误码，并且*pzErrMsg应该指向包含英语错误提示的缓冲区
** message. It is the responsibility of the caller to eventually release            |最后应该调用sqlite3_free() 函数来清除这个缓冲区
** this buffer by calling sqlite3_free().
*/
int sqlite3ParseUri(                                                                /*解析URI*/
  const char *zDefaultVfs,        /* VFS to use if no "vfs=xxx" query option */     /*如果包含"vfs=xxx"查询选项，则使用VFS*/
  const char *zUri,               /* Nul-terminated URI to parse */                 /*没有URI需要解析*/
  unsigned int *pFlags,           /* IN/OUT: SQLITE_OPEN_XXX flags */               /*输入输出：SQLITE_OPEN_XXX 标志*/
  sqlite3_vfs **ppVfs,            /* OUT: VFS to use */                             /*输出：使用VFS*/
  char **pzFile,                  /* OUT: Filename component of URI */              /*输出：URI的文件名成分*/
  char **pzErrMsg                 /* OUT: Error message (if rc!=SQLITE_OK) */       /*输出：当rc!=SQLITE_OK，输出错误信息*/
){
  int rc = SQLITE_OK;
  unsigned int flags = *pFlags;
  const char *zVfs = zDefaultVfs;
  char *zFile;
  char c;
  int nUri = sqlite3Strlen30(zUri);

  assert( *pzErrMsg==0 );

  if( ((flags & SQLITE_OPEN_URI) || sqlite3GlobalConfig.bOpenUri) 
   && nUri>=5 && memcmp(zUri, "file:", 5)==0 
  ){
    char *zOpt;
    int eState;                   /* Parser state when parsing URI */               /*加载URI的加载状态*/
    int iIn;                      /* Input character index */                       /*输入特征索引*/
    int iOut = 0;                 /* Output character index */                      /*输出特征索引*/
    int nByte = nUri+2;           /* Bytes of space to allocate */                  /*分配的字节空间数*/

    /* Make sure the SQLITE_OPEN_URI flag is set to indicate to the VFS xOpen       |确保SQLITE_OPEN_URI标志是用来表示VFS的方法xOpen，可能有额外的参数连接在文件名后
    ** method that there may be extra parameters following the file-name.  */
    flags |= SQLITE_OPEN_URI;

    for(iIn=0; iIn<nUri; iIn++) nByte += (zUri[iIn]=='&');
    zFile = sqlite3_malloc(nByte);
    if( !zFile ) return SQLITE_NOMEM;

    /* Discard the scheme and authority segments of the URI. */
    if( zUri[5]=='/' && zUri[6]=='/' ){
      iIn = 7;
      while( zUri[iIn] && zUri[iIn]!='/' ) iIn++;

      if( iIn!=7 && (iIn!=16 || memcmp("localhost", &zUri[7], 9)) ){
        *pzErrMsg = sqlite3_mprintf("invalid uri authority: %.*s", 
            iIn-7, &zUri[7]);
        rc = SQLITE_ERROR;
        goto parse_uri_out;
      }
    }else{
      iIn = 5;
    }

    /* Copy the filename and any query parameters into the zFile buffer. 
    ** Decode %HH escape codes along the way. 
    **
    ** Within this loop, variable eState may be set to 0, 1 or 2, depending
    ** on the parsing context. As follows:
    **
    **   0: Parsing file-name.
    **   1: Parsing name section of a name=value query parameter.
    **   2: Parsing value section of a name=value query parameter.
    */
    eState = 0;
    while( (c = zUri[iIn])!=0 && c!='#' ){
      iIn++;
      if( c=='%' 
       && sqlite3Isxdigit(zUri[iIn]) 
       && sqlite3Isxdigit(zUri[iIn+1]) 
      ){
        int octet = (sqlite3HexToInt(zUri[iIn++]) << 4);
        octet += sqlite3HexToInt(zUri[iIn++]);

        assert( octet>=0 && octet<256 );
        if( octet==0 ){
          /* This branch is taken when "%00" appears within the URI. In this
          ** case we ignore all text in the remainder of the path, name or
          ** value currently being parsed. So ignore the current character
          ** and skip to the next "?", "=" or "&", as appropriate. */
          while( (c = zUri[iIn])!=0 && c!='#' 
              && (eState!=0 || c!='?')
              && (eState!=1 || (c!='=' && c!='&'))
              && (eState!=2 || c!='&')
          ){
            iIn++;
          }
          continue;
        }
        c = octet;
      }else if( eState==1 && (c=='&' || c=='=') ){
        if( zFile[iOut-1]==0 ){
          /* An empty option name. Ignore this option altogether. */
          while( zUri[iIn] && zUri[iIn]!='#' && zUri[iIn-1]!='&' ) iIn++;
          continue;
        }
        if( c=='&' ){
          zFile[iOut++] = '\0';
        }else{
          eState = 2;
        }
        c = 0;
      }else if( (eState==0 && c=='?') || (eState==2 && c=='&') ){
        c = 0;
        eState = 1;
      }
      zFile[iOut++] = c;
    }
    if( eState==1 ) zFile[iOut++] = '\0';
    zFile[iOut++] = '\0';
    zFile[iOut++] = '\0';

    /* Check if there were any options specified that should be interpreted 
    ** here. Options that are interpreted here include "vfs" and those that
    ** correspond to flags that may be passed to the sqlite3_open_v2()
    ** method. */
    zOpt = &zFile[sqlite3Strlen30(zFile)+1];
    while( zOpt[0] ){
      int nOpt = sqlite3Strlen30(zOpt);
      char *zVal = &zOpt[nOpt+1];
      int nVal = sqlite3Strlen30(zVal);

      if( nOpt==3 && memcmp("vfs", zOpt, 3)==0 ){
        zVfs = zVal;
      }else{
        struct OpenMode {
          const char *z;
          int mode;
        } *aMode = 0;
        char *zModeType = 0;
        int mask = 0;
        int limit = 0;

        if( nOpt==5 && memcmp("cache", zOpt, 5)==0 ){
          static struct OpenMode aCacheMode[] = {
            { "shared",  SQLITE_OPEN_SHAREDCACHE },
            { "private", SQLITE_OPEN_PRIVATECACHE },
            { 0, 0 }
          };

          mask = SQLITE_OPEN_SHAREDCACHE|SQLITE_OPEN_PRIVATECACHE;
          aMode = aCacheMode;
          limit = mask;
          zModeType = "cache";
        }
        if( nOpt==4 && memcmp("mode", zOpt, 4)==0 ){
          static struct OpenMode aOpenMode[] = {
            { "ro",  SQLITE_OPEN_READONLY },
            { "rw",  SQLITE_OPEN_READWRITE }, 
            { "rwc", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE },
            { "memory", SQLITE_OPEN_MEMORY },
            { 0, 0 }
          };

          mask = SQLITE_OPEN_READONLY | SQLITE_OPEN_READWRITE
                   | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY;
          aMode = aOpenMode;
          limit = mask & flags;
          zModeType = "access";
        }

        if( aMode ){
          int i;
          int mode = 0;
          for(i=0; aMode[i].z; i++){
            const char *z = aMode[i].z;
            if( nVal==sqlite3Strlen30(z) && 0==memcmp(zVal, z, nVal) ){
              mode = aMode[i].mode;
              break;
            }
          }
          if( mode==0 ){
            *pzErrMsg = sqlite3_mprintf("no such %s mode: %s", zModeType, zVal);
            rc = SQLITE_ERROR;
            goto parse_uri_out;
          }
          if( (mode & ~SQLITE_OPEN_MEMORY)>limit ){
            *pzErrMsg = sqlite3_mprintf("%s mode not allowed: %s",
                                        zModeType, zVal);
            rc = SQLITE_PERM;
            goto parse_uri_out;
          }
          flags = (flags & ~mask) | mode;
        }
      }

      zOpt = &zVal[nVal+1];
    }

  }else{
    zFile = sqlite3_malloc(nUri+2);
    if( !zFile ) return SQLITE_NOMEM;
    memcpy(zFile, zUri, nUri);
    zFile[nUri] = '\0';
    zFile[nUri+1] = '\0';
    flags &= ~SQLITE_OPEN_URI;
  }

  *ppVfs = sqlite3_vfs_find(zVfs);
  if( *ppVfs==0 ){
    *pzErrMsg = sqlite3_mprintf("no such vfs: %s", zVfs);
    rc = SQLITE_ERROR;
  }
 parse_uri_out:
  if( rc!=SQLITE_OK ){
    sqlite3_free(zFile);
    zFile = 0;
  }
  *pFlags = flags;
  *pzFile = zFile;
  return rc;
}


/*
** This routine does the work of opening a database on behalf of
** sqlite3_open() and sqlite3_open16(). The database filename "zFilename"  
** is UTF-8 encoded.
*/
static int openDatabase(
  const char *zFilename, /* Database filename UTF-8 encoded */
  sqlite3 **ppDb,        /* OUT: Returned database handle */
  unsigned int flags,    /* Operational flags */
  const char *zVfs       /* Name of the VFS to use */
){
  sqlite3 *db;                    /* Store allocated handle here */
  int rc;                         /* Return code */
  int isThreadsafe;               /* True for threadsafe connections */
  char *zOpen = 0;                /* Filename argument to pass to BtreeOpen() */
  char *zErrMsg = 0;              /* Error message from sqlite3ParseUri() */

  *ppDb = 0;
#ifndef SQLITE_OMIT_AUTOINIT
  rc = sqlite3_initialize();
  if( rc ) return rc;
#endif

  /* Only allow sensible combinations of bits in the flags argument.  
  ** Throw an error if any non-sense combination is used.  If we
  ** do not block illegal combinations here, it could trigger
  ** assert() statements in deeper layers.  Sensible combinations
  ** are:
  **
  **  1:  SQLITE_OPEN_READONLY
  **  2:  SQLITE_OPEN_READWRITE
  **  6:  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
  */
  assert( SQLITE_OPEN_READONLY  == 0x01 );
  assert( SQLITE_OPEN_READWRITE == 0x02 );
  assert( SQLITE_OPEN_CREATE    == 0x04 );
  testcase( (1<<(flags&7))==0x02 ); /* READONLY */
  testcase( (1<<(flags&7))==0x04 ); /* READWRITE */
  testcase( (1<<(flags&7))==0x40 ); /* READWRITE | CREATE */
  if( ((1<<(flags&7)) & 0x46)==0 ) return SQLITE_MISUSE_BKPT;

  if( sqlite3GlobalConfig.bCoreMutex==0 ){
    isThreadsafe = 0;
  }else if( flags & SQLITE_OPEN_NOMUTEX ){
    isThreadsafe = 0;
  }else if( flags & SQLITE_OPEN_FULLMUTEX ){
    isThreadsafe = 1;
  }else{
    isThreadsafe = sqlite3GlobalConfig.bFullMutex;
  }
  if( flags & SQLITE_OPEN_PRIVATECACHE ){
    flags &= ~SQLITE_OPEN_SHAREDCACHE;
  }else if( sqlite3GlobalConfig.sharedCacheEnabled ){
    flags |= SQLITE_OPEN_SHAREDCACHE;
  }

  /* Remove harmful bits from the flags parameter
  **
  ** The SQLITE_OPEN_NOMUTEX and SQLITE_OPEN_FULLMUTEX flags were
  ** dealt with in the previous code block.  Besides these, the only
  ** valid input flags for sqlite3_open_v2() are SQLITE_OPEN_READONLY,
  ** SQLITE_OPEN_READWRITE, SQLITE_OPEN_CREATE, SQLITE_OPEN_SHAREDCACHE,
  ** SQLITE_OPEN_PRIVATECACHE, and some reserved bits.  Silently mask
  ** off all other flags.
  */
  flags &=  ~( SQLITE_OPEN_DELETEONCLOSE |
               SQLITE_OPEN_EXCLUSIVE |
               SQLITE_OPEN_MAIN_DB |
               SQLITE_OPEN_TEMP_DB | 
               SQLITE_OPEN_TRANSIENT_DB | 
               SQLITE_OPEN_MAIN_JOURNAL | 
               SQLITE_OPEN_TEMP_JOURNAL | 
               SQLITE_OPEN_SUBJOURNAL | 
               SQLITE_OPEN_MASTER_JOURNAL |
               SQLITE_OPEN_NOMUTEX |
               SQLITE_OPEN_FULLMUTEX |
               SQLITE_OPEN_WAL
             );

  /* Allocate the sqlite data structure */
  db = sqlite3MallocZero( sizeof(sqlite3) );
  if( db==0 ) goto opendb_out;
  if( isThreadsafe ){
    db->mutex = sqlite3MutexAlloc(SQLITE_MUTEX_RECURSIVE);
    if( db->mutex==0 ){
      sqlite3_free(db);
      db = 0;
      goto opendb_out;
    }
  }
  sqlite3_mutex_enter(db->mutex);
  db->errMask = 0xff;
  db->nDb = 2;
  db->magic = SQLITE_MAGIC_BUSY;
  db->aDb = db->aDbStatic;

  assert( sizeof(db->aLimit)==sizeof(aHardLimit) );
  memcpy(db->aLimit, aHardLimit, sizeof(db->aLimit));
  db->autoCommit = 1;
  db->nextAutovac = -1;
  db->nextPagesize = 0;
  db->flags |= SQLITE_ShortColNames | SQLITE_AutoIndex | SQLITE_EnableTrigger
#if SQLITE_DEFAULT_FILE_FORMAT<4
                 | SQLITE_LegacyFileFmt
#endif
#ifdef SQLITE_ENABLE_LOAD_EXTENSION
                 | SQLITE_LoadExtension
#endif
#if SQLITE_DEFAULT_RECURSIVE_TRIGGERS
                 | SQLITE_RecTriggers
#endif
#if defined(SQLITE_DEFAULT_FOREIGN_KEYS) && SQLITE_DEFAULT_FOREIGN_KEYS
                 | SQLITE_ForeignKeys
#endif
      ;
  sqlite3HashInit(&db->aCollSeq);
#ifndef SQLITE_OMIT_VIRTUALTABLE
  sqlite3HashInit(&db->aModule);
#endif

  /* Add the default collation sequence BINARY. BINARY works for both UTF-8
  ** and UTF-16, so add a version for each to avoid any unnecessary
  ** conversions. The only error that can occur here is a malloc() failure.
  */
  createCollation(db, "BINARY", SQLITE_UTF8, 0, binCollFunc, 0);
  createCollation(db, "BINARY", SQLITE_UTF16BE, 0, binCollFunc, 0);
  createCollation(db, "BINARY", SQLITE_UTF16LE, 0, binCollFunc, 0);
  createCollation(db, "RTRIM", SQLITE_UTF8, (void*)1, binCollFunc, 0);
  if( db->mallocFailed ){
    goto opendb_out;
  }
  db->pDfltColl = sqlite3FindCollSeq(db, SQLITE_UTF8, "BINARY", 0);
  assert( db->pDfltColl!=0 );

  /* Also add a UTF-8 case-insensitive collation sequence. */
  createCollation(db, "NOCASE", SQLITE_UTF8, 0, nocaseCollatingFunc, 0);

  /* Parse the filename/URI argument. */
  db->openFlags = flags;
  rc = sqlite3ParseUri(zVfs, zFilename, &flags, &db->pVfs, &zOpen, &zErrMsg);
  if( rc!=SQLITE_OK ){
    if( rc==SQLITE_NOMEM ) db->mallocFailed = 1;
    sqlite3Error(db, rc, zErrMsg ? "%s" : 0, zErrMsg);
    sqlite3_free(zErrMsg);
    goto opendb_out;
  }

  /* Open the backend database driver */
  rc = sqlite3BtreeOpen(db->pVfs, zOpen, db, &db->aDb[0].pBt, 0,
                        flags | SQLITE_OPEN_MAIN_DB);
  if( rc!=SQLITE_OK ){
    if( rc==SQLITE_IOERR_NOMEM ){
      rc = SQLITE_NOMEM;
    }
    sqlite3Error(db, rc, 0);
    goto opendb_out;
  }
  db->aDb[0].pSchema = sqlite3SchemaGet(db, db->aDb[0].pBt);
  db->aDb[1].pSchema = sqlite3SchemaGet(db, 0);


  /* The default safety_level for the main database is 'full'; for the temp
  ** database it is 'NONE'. This matches the pager layer defaults.  
  */
  db->aDb[0].zName = "main";
  db->aDb[0].safety_level = 3;
  db->aDb[1].zName = "temp";
  db->aDb[1].safety_level = 1;

  db->magic = SQLITE_MAGIC_OPEN;
  if( db->mallocFailed ){
    goto opendb_out;
  }

  /* Register all built-in functions, but do not attempt to read the
  ** database schema yet. This is delayed until the first time the database
  ** is accessed.
  */
  sqlite3Error(db, SQLITE_OK, 0);
  sqlite3RegisterBuiltinFunctions(db);

  /* Load automatic extensions - extensions that have been registered
  ** using the sqlite3_automatic_extension() API.
  */
  rc = sqlite3_errcode(db);
  if( rc==SQLITE_OK ){
    sqlite3AutoLoadExtensions(db);
    rc = sqlite3_errcode(db);
    if( rc!=SQLITE_OK ){
      goto opendb_out;
    }
  }

#ifdef SQLITE_ENABLE_FTS1
  if( !db->mallocFailed ){
    extern int sqlite3Fts1Init(sqlite3*);
    rc = sqlite3Fts1Init(db);
  }
#endif

#ifdef SQLITE_ENABLE_FTS2
  if( !db->mallocFailed && rc==SQLITE_OK ){
    extern int sqlite3Fts2Init(sqlite3*);
    rc = sqlite3Fts2Init(db);
  }
#endif

#ifdef SQLITE_ENABLE_FTS3
  if( !db->mallocFailed && rc==SQLITE_OK ){
    rc = sqlite3Fts3Init(db);
  }
#endif

#ifdef SQLITE_ENABLE_ICU
  if( !db->mallocFailed && rc==SQLITE_OK ){
    rc = sqlite3IcuInit(db);
  }
#endif

#ifdef SQLITE_ENABLE_RTREE
  if( !db->mallocFailed && rc==SQLITE_OK){
    rc = sqlite3RtreeInit(db);
  }
#endif

  sqlite3Error(db, rc, 0);

  /* -DSQLITE_DEFAULT_LOCKING_MODE=1 makes EXCLUSIVE the default locking
  ** mode.  -DSQLITE_DEFAULT_LOCKING_MODE=0 make NORMAL the default locking
  ** mode.  Doing nothing at all also makes NORMAL the default.
  */
#ifdef SQLITE_DEFAULT_LOCKING_MODE
  db->dfltLockMode = SQLITE_DEFAULT_LOCKING_MODE;
  sqlite3PagerLockingMode(sqlite3BtreePager(db->aDb[0].pBt),
                          SQLITE_DEFAULT_LOCKING_MODE);
#endif

  /* Enable the lookaside-malloc subsystem */
  setupLookaside(db, 0, sqlite3GlobalConfig.szLookaside,
                        sqlite3GlobalConfig.nLookaside);

  sqlite3_wal_autocheckpoint(db, SQLITE_DEFAULT_WAL_AUTOCHECKPOINT);

opendb_out:
  sqlite3_free(zOpen);
  if( db ){
    assert( db->mutex!=0 || isThreadsafe==0 || sqlite3GlobalConfig.bFullMutex==0 );
    sqlite3_mutex_leave(db->mutex);
  }
  rc = sqlite3_errcode(db);
  assert( db!=0 || rc==SQLITE_NOMEM );
  if( rc==SQLITE_NOMEM ){
    sqlite3_close(db);
    db = 0;
  }else if( rc!=SQLITE_OK ){
    db->magic = SQLITE_MAGIC_SICK;
  }
  *ppDb = db;
  return sqlite3ApiExit(0, rc);
}

/*
** Open a new database handle.
*/
int sqlite3_open(
  const char *zFilename, 
  sqlite3 **ppDb 
){
  return openDatabase(zFilename, ppDb,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
}
int sqlite3_open_v2(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb,         /* OUT: SQLite db handle */
  int flags,              /* Flags */
  const char *zVfs        /* Name of VFS module to use */
){
  return openDatabase(filename, ppDb, (unsigned int)flags, zVfs);
}

#ifndef SQLITE_OMIT_UTF16
/*
** Open a new database handle.
*/
int sqlite3_open16(
  const void *zFilename, 
  sqlite3 **ppDb
){
  char const *zFilename8;   /* zFilename encoded in UTF-8 instead of UTF-16 */
  sqlite3_value *pVal;
  int rc;

  assert( zFilename );
  assert( ppDb );
  *ppDb = 0;
#ifndef SQLITE_OMIT_AUTOINIT
  rc = sqlite3_initialize();
  if( rc ) return rc;
#endif
  pVal = sqlite3ValueNew(0);
  sqlite3ValueSetStr(pVal, -1, zFilename, SQLITE_UTF16NATIVE, SQLITE_STATIC);
  zFilename8 = sqlite3ValueText(pVal, SQLITE_UTF8);
  if( zFilename8 ){
    rc = openDatabase(zFilename8, ppDb,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
    assert( *ppDb || rc==SQLITE_NOMEM );
    if( rc==SQLITE_OK && !DbHasProperty(*ppDb, 0, DB_SchemaLoaded) ){
      ENC(*ppDb) = SQLITE_UTF16NATIVE;
    }
  }else{
    rc = SQLITE_NOMEM;
  }
  sqlite3ValueFree(pVal);

  return sqlite3ApiExit(0, rc);
}
#endif /* SQLITE_OMIT_UTF16 */

/*
** Register a new collation sequence with the database handle db.
*/
int sqlite3_create_collation(
  sqlite3* db, 
  const char *zName, 
  int enc, 
  void* pCtx,
  int(*xCompare)(void*,int,const void*,int,const void*)
){
  int rc;
  sqlite3_mutex_enter(db->mutex);
  assert( !db->mallocFailed );
  rc = createCollation(db, zName, (u8)enc, pCtx, xCompare, 0);
  rc = sqlite3ApiExit(db, rc);
  sqlite3_mutex_leave(db->mutex);
  return rc;
}

/*
** Register a new collation sequence with the database handle db.
*/
int sqlite3_create_collation_v2(
  sqlite3* db, 
  const char *zName, 
  int enc, 
  void* pCtx,
  int(*xCompare)(void*,int,const void*,int,const void*),
  void(*xDel)(void*)
){
  int rc;
  sqlite3_mutex_enter(db->mutex);
  assert( !db->mallocFailed );
  rc = createCollation(db, zName, (u8)enc, pCtx, xCompare, xDel);
  rc = sqlite3ApiExit(db, rc);
  sqlite3_mutex_leave(db->mutex);
  return rc;
}

#ifndef SQLITE_OMIT_UTF16
/*
** Register a new collation sequence with the database handle db.
*/
int sqlite3_create_collation16(
  sqlite3* db, 
  const void *zName,
  int enc, 
  void* pCtx,
  int(*xCompare)(void*,int,const void*,int,const void*)
){
  int rc = SQLITE_OK;
  char *zName8;
  sqlite3_mutex_enter(db->mutex);
  assert( !db->mallocFailed );
  zName8 = sqlite3Utf16to8(db, zName, -1, SQLITE_UTF16NATIVE);
  if( zName8 ){
    rc = createCollation(db, zName8, (u8)enc, pCtx, xCompare, 0);
    sqlite3DbFree(db, zName8);
  }
  rc = sqlite3ApiExit(db, rc);
  sqlite3_mutex_leave(db->mutex);
  return rc;
}
#endif /* SQLITE_OMIT_UTF16 */

/*
** Register a collation sequence factory callback with the database handle
** db. Replace any previously installed collation sequence factory.
*/
int sqlite3_collation_needed(
  sqlite3 *db, 
  void *pCollNeededArg, 
  void(*xCollNeeded)(void*,sqlite3*,int eTextRep,const char*)
){
  sqlite3_mutex_enter(db->mutex);
  db->xCollNeeded = xCollNeeded;
  db->xCollNeeded16 = 0;
  db->pCollNeededArg = pCollNeededArg;
  sqlite3_mutex_leave(db->mutex);
  return SQLITE_OK;
}

#ifndef SQLITE_OMIT_UTF16
/*
** Register a collation sequence factory callback with the database handle
** db. Replace any previously installed collation sequence factory.
*/
int sqlite3_collation_needed16(
  sqlite3 *db, 
  void *pCollNeededArg, 
  void(*xCollNeeded16)(void*,sqlite3*,int eTextRep,const void*)
){
  sqlite3_mutex_enter(db->mutex);
  db->xCollNeeded = 0;
  db->xCollNeeded16 = xCollNeeded16;
  db->pCollNeededArg = pCollNeededArg;
  sqlite3_mutex_leave(db->mutex);
  return SQLITE_OK;
}
#endif /* SQLITE_OMIT_UTF16 */

#ifndef SQLITE_OMIT_DEPRECATED
/*
** This function is now an anachronism. It used to be used to recover from a
** malloc() failure, but SQLite now does this automatically.
*/
int sqlite3_global_recover(void){
  return SQLITE_OK;
}
#endif

/*
** Test to see whether or not the database connection is in autocommit
** mode.  Return TRUE if it is and FALSE if not.  Autocommit mode is on
** by default.  Autocommit is disabled by a BEGIN statement and reenabled
** by the next COMMIT or ROLLBACK.
**
******* THIS IS AN EXPERIMENTAL API AND IS SUBJECT TO CHANGE ******
*/
int sqlite3_get_autocommit(sqlite3 *db){
  return db->autoCommit;
}

/*
** The following routines are subtitutes for constants SQLITE_CORRUPT,
** SQLITE_MISUSE, SQLITE_CANTOPEN, SQLITE_IOERR and possibly other error
** constants.  They server two purposes:
**
**   1.  Serve as a convenient place to set a breakpoint in a debugger
**       to detect when version error conditions occurs.
**
**   2.  Invoke sqlite3_log() to provide the source code location where
**       a low-level error is first detected.
*/
int sqlite3CorruptError(int lineno){
  testcase( sqlite3GlobalConfig.xLog!=0 );
  sqlite3_log(SQLITE_CORRUPT,
              "database corruption at line %d of [%.10s]",
              lineno, 20+sqlite3_sourceid());
  return SQLITE_CORRUPT;
}
int sqlite3MisuseError(int lineno){
  testcase( sqlite3GlobalConfig.xLog!=0 );
  sqlite3_log(SQLITE_MISUSE, 
              "misuse at line %d of [%.10s]",
              lineno, 20+sqlite3_sourceid());
  return SQLITE_MISUSE;
}
int sqlite3CantopenError(int lineno){
  testcase( sqlite3GlobalConfig.xLog!=0 );
  sqlite3_log(SQLITE_CANTOPEN, 
              "cannot open file at line %d of [%.10s]",
              lineno, 20+sqlite3_sourceid());
  return SQLITE_CANTOPEN;
}


#ifndef SQLITE_OMIT_DEPRECATED
/*
** This is a convenience routine that makes sure that all thread-specific
** data for this thread has been deallocated.
**
** SQLite no longer uses thread-specific data so this routine is now a
** no-op.  It is retained for historical compatibility.
*/
void sqlite3_thread_cleanup(void){
}
#endif

/*
** Return meta information about a specific column of a database table.
** See comment in sqlite3.h (sqlite.h.in) for details.
*/
#ifdef SQLITE_ENABLE_COLUMN_METADATA
int sqlite3_table_column_metadata(
  sqlite3 *db,                /* Connection handle */
  const char *zDbName,        /* Database name or NULL */
  const char *zTableName,     /* Table name */
  const char *zColumnName,    /* Column name */
  char const **pzDataType,    /* OUTPUT: Declared data type */
  char const **pzCollSeq,     /* OUTPUT: Collation sequence name */
  int *pNotNull,              /* OUTPUT: True if NOT NULL constraint exists */
  int *pPrimaryKey,           /* OUTPUT: True if column part of PK */
  int *pAutoinc               /* OUTPUT: True if column is auto-increment */
){
  int rc;
  char *zErrMsg = 0;
  Table *pTab = 0;
  Column *pCol = 0;
  int iCol;

  char const *zDataType = 0;
  char const *zCollSeq = 0;
  int notnull = 0;
  int primarykey = 0;
  int autoinc = 0;

  /* Ensure the database schema has been loaded */
  sqlite3_mutex_enter(db->mutex);
  sqlite3BtreeEnterAll(db);
  rc = sqlite3Init(db, &zErrMsg);
  if( SQLITE_OK!=rc ){
    goto error_out;
  }

  /* Locate the table in question */
  pTab = sqlite3FindTable(db, zTableName, zDbName);
  if( !pTab || pTab->pSelect ){
    pTab = 0;
    goto error_out;
  }

  /* Find the column for which info is requested */
  if( sqlite3IsRowid(zColumnName) ){
    iCol = pTab->iPKey;
    if( iCol>=0 ){
      pCol = &pTab->aCol[iCol];
    }
  }else{
    for(iCol=0; iCol<pTab->nCol; iCol++){
      pCol = &pTab->aCol[iCol];
      if( 0==sqlite3StrICmp(pCol->zName, zColumnName) ){
        break;
      }
    }
    if( iCol==pTab->nCol ){
      pTab = 0;
      goto error_out;
    }
  }

  /* The following block stores the meta information that will be returned
  ** to the caller in local variables zDataType, zCollSeq, notnull, primarykey
  ** and autoinc. At this point there are two possibilities:
  ** 
  **     1. The specified column name was rowid", "oid" or "_rowid_" 
  **        and there is no explicitly declared IPK column. 
  **
  **     2. The table is not a view and the column name identified an 
  **        explicitly declared column. Copy meta information from *pCol.
  */ 
  if( pCol ){
    zDataType = pCol->zType;
    zCollSeq = pCol->zColl;
    notnull = pCol->notNull!=0;
    primarykey  = pCol->isPrimKey!=0;
    autoinc = pTab->iPKey==iCol && (pTab->tabFlags & TF_Autoincrement)!=0;
  }else{
    zDataType = "INTEGER";
    primarykey = 1;
  }
  if( !zCollSeq ){
    zCollSeq = "BINARY";
  }

error_out:
  sqlite3BtreeLeaveAll(db);

  /* Whether the function call succeeded or failed, set the output parameters
  ** to whatever their local counterparts contain. If an error did occur,
  ** this has the effect of zeroing all output parameters.
  */
  if( pzDataType ) *pzDataType = zDataType;
  if( pzCollSeq ) *pzCollSeq = zCollSeq;
  if( pNotNull ) *pNotNull = notnull;
  if( pPrimaryKey ) *pPrimaryKey = primarykey;
  if( pAutoinc ) *pAutoinc = autoinc;

  if( SQLITE_OK==rc && !pTab ){
    sqlite3DbFree(db, zErrMsg);
    zErrMsg = sqlite3MPrintf(db, "no such table column: %s.%s", zTableName,
        zColumnName);
    rc = SQLITE_ERROR;
  }
  sqlite3Error(db, rc, (zErrMsg?"%s":0), zErrMsg);
  sqlite3DbFree(db, zErrMsg);
  rc = sqlite3ApiExit(db, rc);
  sqlite3_mutex_leave(db->mutex);
  return rc;
}
#endif

/*
** Sleep for a little while.  Return the amount of time slept.
*/
int sqlite3_sleep(int ms){
  sqlite3_vfs *pVfs;
  int rc;
  pVfs = sqlite3_vfs_find(0);
  if( pVfs==0 ) return 0;

  /* This function works in milliseconds, but the underlying OsSleep() 
  ** API uses microseconds. Hence the 1000's.
  */
  rc = (sqlite3OsSleep(pVfs, 1000*ms)/1000);
  return rc;
}

/*
** Enable or disable the extended result codes.
*/
int sqlite3_extended_result_codes(sqlite3 *db, int onoff){
  sqlite3_mutex_enter(db->mutex);
  db->errMask = onoff ? 0xffffffff : 0xff;
  sqlite3_mutex_leave(db->mutex);
  return SQLITE_OK;
}

/*
** Invoke the xFileControl method on a particular database.
*/
int sqlite3_file_control(sqlite3 *db, const char *zDbName, int op, void *pArg){
  int rc = SQLITE_ERROR;
  Btree *pBtree;

  sqlite3_mutex_enter(db->mutex);
  pBtree = sqlite3DbNameToBtree(db, zDbName);
  if( pBtree ){
    Pager *pPager;
    sqlite3_file *fd;
    sqlite3BtreeEnter(pBtree);
    pPager = sqlite3BtreePager(pBtree);
    assert( pPager!=0 );
    fd = sqlite3PagerFile(pPager);
    assert( fd!=0 );
    if( op==SQLITE_FCNTL_FILE_POINTER ){
      *(sqlite3_file**)pArg = fd;
      rc = SQLITE_OK;
    }else if( fd->pMethods ){
      rc = sqlite3OsFileControl(fd, op, pArg);
    }else{
      rc = SQLITE_NOTFOUND;
    }
    sqlite3BtreeLeave(pBtree);
  }
  sqlite3_mutex_leave(db->mutex);
  return rc;   
}

/*
** Interface to the testing logic.
*/
int sqlite3_test_control(int op, ...){
  int rc = 0;
#ifndef SQLITE_OMIT_BUILTIN_TEST
  va_list ap;
  va_start(ap, op);
  switch( op ){

    /*
    ** Save the current state of the PRNG.
    */
    case SQLITE_TESTCTRL_PRNG_SAVE: {
      sqlite3PrngSaveState();
      break;
    }

    /*
    ** Restore the state of the PRNG to the last state saved using
    ** PRNG_SAVE.  If PRNG_SAVE has never before been called, then
    ** this verb acts like PRNG_RESET.
    */
    case SQLITE_TESTCTRL_PRNG_RESTORE: {
      sqlite3PrngRestoreState();
      break;
    }

    /*
    ** Reset the PRNG back to its uninitialized state.  The next call
    ** to sqlite3_randomness() will reseed the PRNG using a single call
    ** to the xRandomness method of the default VFS.
    */
    case SQLITE_TESTCTRL_PRNG_RESET: {
      sqlite3PrngResetState();
      break;
    }

    /*
    **  sqlite3_test_control(BITVEC_TEST, size, program)
    **
    ** Run a test against a Bitvec object of size.  The program argument
    ** is an array of integers that defines the test.  Return -1 on a
    ** memory allocation error, 0 on success, or non-zero for an error.
    ** See the sqlite3BitvecBuiltinTest() for additional information.
    */
    case SQLITE_TESTCTRL_BITVEC_TEST: {
      int sz = va_arg(ap, int);
      int *aProg = va_arg(ap, int*);
      rc = sqlite3BitvecBuiltinTest(sz, aProg);
      break;
    }

    /*
    **  sqlite3_test_control(BENIGN_MALLOC_HOOKS, xBegin, xEnd)
    **
    ** Register hooks to call to indicate which malloc() failures 
    ** are benign.
    */
    case SQLITE_TESTCTRL_BENIGN_MALLOC_HOOKS: {
      typedef void (*void_function)(void);
      void_function xBenignBegin;
      void_function xBenignEnd;
      xBenignBegin = va_arg(ap, void_function);
      xBenignEnd = va_arg(ap, void_function);
      sqlite3BenignMallocHooks(xBenignBegin, xBenignEnd);
      break;
    }

    /*
    **  sqlite3_test_control(SQLITE_TESTCTRL_PENDING_BYTE, unsigned int X)
    **
    ** Set the PENDING byte to the value in the argument, if X>0.
    ** Make no changes if X==0.  Return the value of the pending byte
    ** as it existing before this routine was called.
    **
    ** IMPORTANT:  Changing the PENDING byte from 0x40000000 results in
    ** an incompatible database file format.  Changing the PENDING byte
    ** while any database connection is open results in undefined and
    ** dileterious behavior.
    */
    case SQLITE_TESTCTRL_PENDING_BYTE: {
      rc = PENDING_BYTE;
#ifndef SQLITE_OMIT_WSD
      {
        unsigned int newVal = va_arg(ap, unsigned int);
        if( newVal ) sqlite3PendingByte = newVal;
      }
#endif
      break;
    }

    /*
    **  sqlite3_test_control(SQLITE_TESTCTRL_ASSERT, int X)
    **
    ** This action provides a run-time test to see whether or not
    ** assert() was enabled at compile-time.  If X is true and assert()
    ** is enabled, then the return value is true.  If X is true and
    ** assert() is disabled, then the return value is zero.  If X is
    ** false and assert() is enabled, then the assertion fires and the
    ** process aborts.  If X is false and assert() is disabled, then the
    ** return value is zero.
    */
    case SQLITE_TESTCTRL_ASSERT: {
      volatile int x = 0;
      assert( (x = va_arg(ap,int))!=0 );
      rc = x;
      break;
    }


    /*
    **  sqlite3_test_control(SQLITE_TESTCTRL_ALWAYS, int X)
    **
    ** This action provides a run-time test to see how the ALWAYS and
    ** NEVER macros were defined at compile-time.
    **
    ** The return value is ALWAYS(X).  
    **
    ** The recommended test is X==2.  If the return value is 2, that means
    ** ALWAYS() and NEVER() are both no-op pass-through macros, which is the
    ** default setting.  If the return value is 1, then ALWAYS() is either
    ** hard-coded to true or else it asserts if its argument is false.
    ** The first behavior (hard-coded to true) is the case if
    ** SQLITE_TESTCTRL_ASSERT shows that assert() is disabled and the second
    ** behavior (assert if the argument to ALWAYS() is false) is the case if
    ** SQLITE_TESTCTRL_ASSERT shows that assert() is enabled.
    **
    ** The run-time test procedure might look something like this:
    **
    **    if( sqlite3_test_control(SQLITE_TESTCTRL_ALWAYS, 2)==2 ){
    **      // ALWAYS() and NEVER() are no-op pass-through macros
    **    }else if( sqlite3_test_control(SQLITE_TESTCTRL_ASSERT, 1) ){
    **      // ALWAYS(x) asserts that x is true. NEVER(x) asserts x is false.
    **    }else{
    **      // ALWAYS(x) is a constant 1.  NEVER(x) is a constant 0.
    **    }
    */
    case SQLITE_TESTCTRL_ALWAYS: {
      int x = va_arg(ap,int);
      rc = ALWAYS(x);
      break;
    }

    /*   sqlite3_test_control(SQLITE_TESTCTRL_RESERVE, sqlite3 *db, int N)
    **
    ** Set the nReserve size to N for the main database on the database
    ** connection db.
    */
    case SQLITE_TESTCTRL_RESERVE: {
      sqlite3 *db = va_arg(ap, sqlite3*);
      int x = va_arg(ap,int);
      sqlite3_mutex_enter(db->mutex);
      sqlite3BtreeSetPageSize(db->aDb[0].pBt, 0, x, 0);
      sqlite3_mutex_leave(db->mutex);
      break;
    }

    /*  sqlite3_test_control(SQLITE_TESTCTRL_OPTIMIZATIONS, sqlite3 *db, int N)
    **
    ** Enable or disable various optimizations for testing purposes.  The 
    ** argument N is a bitmask of optimizations to be disabled.  For normal
    ** operation N should be 0.  The idea is that a test program (like the
    ** SQL Logic Test or SLT test module) can run the same SQL multiple times
    ** with various optimizations disabled to verify that the same answer
    ** is obtained in every case.
    */
    case SQLITE_TESTCTRL_OPTIMIZATIONS: {
      sqlite3 *db = va_arg(ap, sqlite3*);
      int x = va_arg(ap,int);
      db->flags = (x & SQLITE_OptMask) | (db->flags & ~SQLITE_OptMask);
      break;
    }

#ifdef SQLITE_N_KEYWORD
    /* sqlite3_test_control(SQLITE_TESTCTRL_ISKEYWORD, const char *zWord)
    **
    ** If zWord is a keyword recognized by the parser, then return the
    ** number of keywords.  Or if zWord is not a keyword, return 0.
    ** 
    ** This test feature is only available in the amalgamation since
    ** the SQLITE_N_KEYWORD macro is not defined in this file if SQLite
    ** is built using separate source files.
    */
    case SQLITE_TESTCTRL_ISKEYWORD: {
      const char *zWord = va_arg(ap, const char*);
      int n = sqlite3Strlen30(zWord);
      rc = (sqlite3KeywordCode((u8*)zWord, n)!=TK_ID) ? SQLITE_N_KEYWORD : 0;
      break;
    }
#endif 

    /* sqlite3_test_control(SQLITE_TESTCTRL_SCRATCHMALLOC, sz, &pNew, pFree);
    **
    ** Pass pFree into sqlite3ScratchFree(). 
    ** If sz>0 then allocate a scratch buffer into pNew.  
    */
    case SQLITE_TESTCTRL_SCRATCHMALLOC: {
      void *pFree, **ppNew;
      int sz;
      sz = va_arg(ap, int);
      ppNew = va_arg(ap, void**);
      pFree = va_arg(ap, void*);
      if( sz ) *ppNew = sqlite3ScratchMalloc(sz);
      sqlite3ScratchFree(pFree);
      break;
    }

    /*   sqlite3_test_control(SQLITE_TESTCTRL_LOCALTIME_FAULT, int onoff);
    **
    ** If parameter onoff is non-zero, configure the wrappers so that all
    ** subsequent calls to localtime() and variants fail. If onoff is zero,
    ** undo this setting.
    */
    case SQLITE_TESTCTRL_LOCALTIME_FAULT: {
      sqlite3GlobalConfig.bLocaltimeFault = va_arg(ap, int);
      break;
    }

#if defined(SQLITE_ENABLE_TREE_EXPLAIN)
    /*   sqlite3_test_control(SQLITE_TESTCTRL_EXPLAIN_STMT,
    **                        sqlite3_stmt*,const char**);
    **
    ** If compiled with SQLITE_ENABLE_TREE_EXPLAIN, each sqlite3_stmt holds
    ** a string that describes the optimized parse tree.  This test-control
    ** returns a pointer to that string.
    */
    case SQLITE_TESTCTRL_EXPLAIN_STMT: {
      sqlite3_stmt *pStmt = va_arg(ap, sqlite3_stmt*);
      const char **pzRet = va_arg(ap, const char**);
      *pzRet = sqlite3VdbeExplanation((Vdbe*)pStmt);
      break;
    }
#endif

  }
  va_end(ap);
#endif /* SQLITE_OMIT_BUILTIN_TEST */
  return rc;
}

/*
** This is a utility routine, useful to VFS implementations, that checks
** to see if a database file was a URI that contained a specific query 
** parameter, and if so obtains the value of the query parameter.
**
** The zFilename argument is the filename pointer passed into the xOpen()
** method of a VFS implementation.  The zParam argument is the name of the
** query parameter we seek.  This routine returns the value of the zParam
** parameter if it exists.  If the parameter does not exist, this routine
** returns a NULL pointer.
*/
const char *sqlite3_uri_parameter(const char *zFilename, const char *zParam){
  if( zFilename==0 ) return 0;
  zFilename += sqlite3Strlen30(zFilename) + 1;
  while( zFilename[0] ){
    int x = strcmp(zFilename, zParam);
    zFilename += sqlite3Strlen30(zFilename) + 1;
    if( x==0 ) return zFilename;
    zFilename += sqlite3Strlen30(zFilename) + 1;
  }
  return 0;
}

/*
** Return a boolean value for a query parameter.
*/
int sqlite3_uri_boolean(const char *zFilename, const char *zParam, int bDflt){
  const char *z = sqlite3_uri_parameter(zFilename, zParam);
  bDflt = bDflt!=0;
  return z ? sqlite3GetBoolean(z, bDflt) : bDflt;
}

/*
** Return a 64-bit integer value for a query parameter.
*/
sqlite3_int64 sqlite3_uri_int64(
  const char *zFilename,    /* Filename as passed to xOpen */
  const char *zParam,       /* URI parameter sought */
  sqlite3_int64 bDflt       /* return if parameter is missing */
){
  const char *z = sqlite3_uri_parameter(zFilename, zParam);
  sqlite3_int64 v;
  if( z && sqlite3Atoi64(z, &v, sqlite3Strlen30(z), SQLITE_UTF8)==SQLITE_OK ){
    bDflt = v;
  }
  return bDflt;
}

/*
** Return the Btree pointer identified by zDbName.  Return NULL if not found.
*/
Btree *sqlite3DbNameToBtree(sqlite3 *db, const char *zDbName){
  int i;
  for(i=0; i<db->nDb; i++){
    if( db->aDb[i].pBt
     && (zDbName==0 || sqlite3StrICmp(zDbName, db->aDb[i].zName)==0)
    ){
      return db->aDb[i].pBt;
    }
  }
  return 0;
}

/*
** Return the filename of the database associated with a database
** connection.
*/
const char *sqlite3_db_filename(sqlite3 *db, const char *zDbName){
  Btree *pBt = sqlite3DbNameToBtree(db, zDbName);
  return pBt ? sqlite3BtreeGetFilename(pBt) : 0;
}

/*
** Return 1 if database is read-only or 0 if read/write.  Return -1 if
** no such database exists.
*/
int sqlite3_db_readonly(sqlite3 *db, const char *zDbName){
  Btree *pBt = sqlite3DbNameToBtree(db, zDbName);
  return pBt ? sqlite3PagerIsreadonly(sqlite3BtreePager(pBt)) : -1;
}
