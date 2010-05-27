/* User. (C) 2005-2010 Niall Douglas

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifdef ENABLE_USERMODEPAGEALLOCATOR

#include "nedtrie.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define REGION_ENTRY(type)                        NEDTRIE_ENTRY(type)
#define REGION_HEAD(name, type)                   NEDTRIE_HEAD(name, type)
#define REGION_INIT(treevar)                      NEDTRIE_INIT(treevar)
#define REGION_EMPTY(treevar)                     NEDTRIE_EMPTY(treevar)
#define REGION_GENERATE(proto, treetype, nodetype, link, cmpfunct) NEDTRIE_GENERATE(proto, treetype, nodetype, link, cmpfunct, NEDTRIE_NOBBLEZEROS(treetype))
#define REGION_INSERT(treetype, treevar, node)    NEDTRIE_INSERT(treetype, treevar, node)
#define REGION_REMOVE(treetype, treevar, node)    NEDTRIE_REMOVE(treetype, treevar, node)
#define REGION_FIND(treetype, treevar, node)      NEDTRIE_FIND(treetype, treevar, node)
#define REGION_NFIND(treetype, treevar, node)     NEDTRIE_NFIND(treetype, treevar, node)
#define REGION_MAX(treetype, treevar)             NEDTRIE_MAX(treetype, treevar)
#define REGION_MIN(treetype, treevar)             NEDTRIE_MIN(treetype, treevar)
#define REGION_NEXT(treetype, treevar, node)      NEDTRIE_NEXT(treetype, treevar, node)
#define REGION_PREV(treetype, treevar, node)      NEDTRIE_PREV(treetype, treevar, node)
#define REGION_FOREACH(var, treetype, treevar)    NEDTRIE_FOREACH(var, treetype, treevar)
#define REGION_HASNODEHEADER(treevar, node, link) NEDTRIE_HASNODEHEADER(treevar, node, link)

#define FLAG_TOPDOWN  0x1
#define FLAG_NOCOMMIT 0x2

typedef struct region_node_s region_node_t;
struct region_node_s {
    REGION_ENTRY(region_node_s) linkS; /* by start addr */
    REGION_ENTRY(region_node_s) linkE; /* by end addr */
    REGION_ENTRY(region_node_s) linkL; /* by length */
    void *start, *end;
};
typedef struct regionS_tree_s regionS_tree_t;
REGION_HEAD(regionS_tree_s, region_node_s);
typedef struct regionE_tree_s regionE_tree_t;
REGION_HEAD(regionE_tree_s, region_node_s);
typedef struct regionL_tree_s regionL_tree_t;
REGION_HEAD(regionL_tree_s, region_node_s);

static size_t regionkeyS(const region_node_t *RESTRICT r)
{
  return (size_t) r->start;
}
static size_t regionkeyE(const region_node_t *RESTRICT r)
{
  return (size_t) r->end;
}
static size_t regionkeyL(const region_node_t *RESTRICT r)
{
  return (size_t) r->end - (size_t) r->start;
}
REGION_GENERATE(static, regionS_tree_s, region_node_s, linkS, regionkeyS);
REGION_GENERATE(static, regionE_tree_s, region_node_s, linkE, regionkeyE);
REGION_GENERATE(static, regionL_tree_s, region_node_s, linkL, regionkeyL);
typedef struct MemorySource_t MemorySource;
static struct MemorySource_t
{
  regionS_tree_t regiontreeS; /* The list of free regions, keyed by start addr */
  regionE_tree_t regiontreeE; /* The list of free regions, keyed by end addr */
  regionL_tree_t regiontreeL; /* The list of free regions, keyed by length */
} lower, upper;

typedef struct OSAddressSpaceReservationData_t
{
  void *addr;
  void *data[2];
} OSAddressSpaceReservationData;
#ifndef WIN32
typedef size_t PageFrameType;

/* This function determines whether the host OS allows user mode physical memory
page mapping. */
static int OSDeterminePhysicalPageSupport(void);

/* This function could ask the host OS for address space, or on embedded systems
it could simply parcel out space via moving a pointer. The second two void *
are some arbitrary extra data to be later passed to OSReleaseAddrSpace(). */
static OSAddressSpaceReservationData OSReserveAddrSpace(size_t space);

/* This function returns address space previously allocated using
OSReserveAddrSpace(). It is guaranteed to exactly match what was previously
returned by that function. */
static int OSReleaseAddrSpace(OSAddressSpaceReservationData *data, size_t space);

/* This function obtains physical memory pages, either by asking the host OS
or on embedded systems by simply pulling them from a free page ring list. */
static size_t OSObtainMemoryPages(PageFrameType *buffer, size_t number, OSAddressSpaceReservationData *data);

/* This function returns previously obtained physical memory pages. */
static size_t OSReleaseMemoryPages(PageFrameType *buffer, size_t number, OSAddressSpaceReservationData *data);

/* This function causes the specified set of physical memory pages to be
mapped at the specified address. On an embedded system this would simply
modify the MMU and flush the appropriate TLB entries.
*/
static size_t OSRemapMemoryPagesOntoAddr(void *addr, size_t entries, PageFrameType *pageframes, OSAddressSpaceReservationData *data);

/* This function causes the specified set of physical memory pages to be
mapped at the specified set of addresses. On an embedded system this would
simply modify the MMU and flush the appropriate TLB entries. It works like this:

for(size_t n=0; n<entries; n++, addrs++, pageframes++) {
  if(*pageframe)
    Map(*addr, *pageframe);
  else
    Unmap(*addr);
}
*/
static size_t OSRemapMemoryPagesOntoAddrs(void **addrs, size_t entries, PageFrameType *pageframes, OSAddressSpaceReservationData *data);
#else
static enum {
  DISABLEEVERYTHING=1,
  HAVEPHYSICALPAGESUPPORT=4,
  NOPHYSICALPAGESUPPORT=5
} PhysicalPageSupport;

#ifdef ENABLE_PHYSICALPAGEEMULATION
/* Windows has the curious problem of using 4Kb pages but requiring those pages
to be mapped at 64Kb aligned address. By far the easiest solution is to pretend
that we actually have 64Kb pages. */
#undef PAGE_SIZE
#define PAGE_SIZE 65536
typedef struct PageFrameType_t
{
  ULONG_PTR pages[16];
} PageFrameType;
#else
typedef ULONG_PTR PageFrameType;
#endif


static int OSDeterminePhysicalPageSupport(void)
{
  if(!PhysicalPageSupport)
  { /* Quick test */
    PageFrameType pageframe;
    size_t no=sizeof(PageFrameType)/sizeof(ULONG_PTR);
    SYSTEM_INFO si={0};
    if(AllocateUserPhysicalPages((HANDLE)-1, (PULONG_PTR) &no, (PULONG_PTR) &pageframe))
    {
      FreeUserPhysicalPages((HANDLE)-1, (PULONG_PTR) &no, (PULONG_PTR) &pageframe);
      PhysicalPageSupport=HAVEPHYSICALPAGESUPPORT;
    }
    else
      PhysicalPageSupport=NOPHYSICALPAGESUPPORT;
    GetSystemInfo(&si);
#ifdef ENABLE_PHYSICALPAGEEMULATION
    if(si.dwAllocationGranularity!=PAGE_SIZE)
    {
      assert(si.dwAllocationGranularity==PAGE_SIZE);
      fprintf(stderr, "User Mode Page Allocator: Allocation granularity is %u not %u. Please recompile with corrected PAGE_SIZE\n", si.dwAllocationGranularity, PAGE_SIZE);
      PhysicalPageSupport=DISABLEEVERYTHING;
    }
    if(si.dwAllocationGranularity/si.dwPageSize!=sizeof(PageFrameType)/sizeof(ULONG_PTR))
    {
      assert(si.dwAllocationGranularity/si.dwPageSize==sizeof(PageFrameType)/sizeof(ULONG_PTR));
      fprintf(stderr, "User Mode Page Allocator: Pages per PageFrameType is %u not %u. Please recompile with corrected PageFrameType definition\n", si.dwAllocationGranularity/si.dwPageSize, sizeof(PageFrameType)/sizeof(ULONG_PTR));
      PhysicalPageSupport=DISABLEEVERYTHING;
    }
#else
    if(si.dwPageSize!=PAGE_SIZE)
    {
      assert(si.dwPageSize==PAGE_SIZE);
      fprintf(stderr, "User Mode Page Allocator: Page size is %u not %u. Please recompile with corrected PAGE_SIZE\n", si.dwPageSize, PAGE_SIZE);
      PhysicalPageSupport=DISABLEEVERYTHING;
    }
#endif
  }
  return PhysicalPageSupport;
}
static OSAddressSpaceReservationData OSReserveAddrSpace(size_t space)
{
  OSAddressSpaceReservationData ret={0};
  if(!PhysicalPageSupport) OSDeterminePhysicalPageSupport();
  if(DISABLEEVERYTHING==PhysicalPageSupport) return ret;
  if(HAVEPHYSICALPAGESUPPORT==PhysicalPageSupport)
  {
    ret.addr=VirtualAlloc(NULL, space, MEM_RESERVE|MEM_PHYSICAL, PAGE_READWRITE);
  }
#ifdef ENABLE_PHYSICALPAGEEMULATION
  if(!ret.addr)
  {
    HANDLE fmh;
    fmh = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE|SEC_RESERVE,
#if defined(_M_IA64) || defined(_M_X64) || defined(WIN64)
                            (DWORD)(space>>32),
#else
                            0,
#endif
                            (DWORD)(space&((DWORD)-1)), NULL);
    if(fmh)
    { /* This is breathtakingly inefficient, but win32 leaves us no choice :(
         At least this function is called very infrequently. */
      while((ret.addr=VirtualAlloc(NULL, space, MEM_RESERVE, PAGE_READWRITE)))
      {
        void *RESTRICT seg;
        VirtualFree(ret.addr, 0, MEM_RELEASE);
        for(seg=ret.addr; seg<(void*)((size_t) ret.addr + space); seg=(void *)((size_t) seg + Win32granularity))
        {
          if(!VirtualAlloc(seg, Win32granularity, MEM_RESERVE, PAGE_READWRITE))
            break;
        }
        if(seg==(void*)((size_t) ret.addr + space))
          break;
        else
        {
          for(; seg>=ret.addr; seg=(void *)((size_t) seg - Win32granularity))
            VirtualFree(seg, 0, MEM_RELEASE);
        }
      }
      if(!ret.addr)
        CloseHandle(fmh);
      else
      {
        ret.data[0]=(void *) fmh;
        ret.data[1]=(void *)(size_t) 1;
      }
    }
  }
#endif
  return ret;
}
static int OSReleaseAddrSpace(OSAddressSpaceReservationData *RESTRICT data, size_t space)
{
  if(!data->data[0])
    return VirtualFree(data->addr, 0, MEM_RELEASE);
#ifdef ENABLE_PHYSICALPAGEEMULATION
  else
  {
    void *seg;
    CloseHandle((HANDLE)data->data[0]);
    for(seg=data->addr; seg<(void*)((size_t) data->addr + space); seg=(void *)((size_t) seg + Win32granularity))
      VirtualFree(seg, 0, MEM_RELEASE);
    return 1;
  }
#endif
  return 0;
}
static size_t OSObtainMemoryPages(PageFrameType *RESTRICT buffer, size_t number, OSAddressSpaceReservationData *RESTRICT data)
{
  if(!data->data[0])
  {
#ifdef ENABLE_PHYSICALPAGEEMULATION
    number*=sizeof(PageFrameType)/sizeof(ULONG_PTR);
#endif
    if(!AllocateUserPhysicalPages((HANDLE)-1, (PULONG_PTR) &number, (PULONG_PTR) buffer))
      return 0;
#ifdef ENABLE_PHYSICALPAGEEMULATION
    number/=sizeof(PageFrameType)/sizeof(ULONG_PTR);
#endif
    return number;
  }
#ifdef ENABLE_PHYSICALPAGEEMULATION
  else
  {
    size_t n;
    ULONG_PTR *RESTRICT pf=(ULONG_PTR *RESTRICT) &data->data[1];
    for(n=0; n<number*(sizeof(PageFrameType)/sizeof(ULONG_PTR)); n++)
      ((ULONG_PTR *) buffer)[n]=(*pf)++;
    return number;
  }
#endif
  return 0;
}
static size_t OSReleaseMemoryPages(PageFrameType *RESTRICT buffer, size_t number, OSAddressSpaceReservationData *RESTRICT data)
{
  size_t n;
  if(!data->data[0])
  {
#ifdef ENABLE_PHYSICALPAGEEMULATION
    number*=sizeof(PageFrameType)/sizeof(ULONG_PTR);
#endif
    if(!FreeUserPhysicalPages((HANDLE)-1, (PULONG_PTR) &number, (PULONG_PTR) buffer)) return 0;
#ifdef ENABLE_PHYSICALPAGEEMULATION
    number/=sizeof(PageFrameType)/sizeof(ULONG_PTR);
#endif
    for(n=0; n<number*(sizeof(PageFrameType)/sizeof(ULONG_PTR)); n++)
      ((ULONG_PTR *) buffer)[n]=0;
    return number;
  }
  /* Always fail if we are emulating physical pages */
  return 0;
}
static size_t OSRemapMemoryPagesOntoAddr(void *addr, size_t entries, PageFrameType *RESTRICT pageframes, OSAddressSpaceReservationData *RESTRICT data)
{
  if(!data->data[0])
  {
#ifdef ENABLE_PHYSICALPAGEEMULATION
    entries*=sizeof(PageFrameType)/sizeof(ULONG_PTR);
#endif
    return MapUserPhysicalPages(addr, entries, (PULONG_PTR) pageframes);
  }
#ifdef ENABLE_PHYSICALPAGEEMULATION
  else
  {
    size_t n, ret=1;
    PageFrameType *RESTRICT pfa, *RESTRICT pf;
    for(n=0; n<entries; n++, addr=(void *)((size_t) addr + PAGE_SIZE), pageframes++)
    {
      if(*pageframe)
      {
        size_t filemappingoffset=PAGE_SIZE*((*pageframe)-1);
        /* Change reservation for next segment */
        if(!VirtualFree(addr, 0, MEM_RELEASE)) ret=0;
        if(!MapViewOfFileEx((HANDLE) data.data[0], FILE_MAP_ALL_ACCESS,
#if defined(_M_IA64) || defined(_M_X64) || defined(WIN64)
                           (DWORD)(filemappingoffset>>32),
#else
                           0,
#endif
                           (DWORD)(filemappingoffset & (DWORD)-1), PAGE_SIZE, addr)) ret=0;
      }
      else
      {
        if(!UnmapViewOfFile(addr)) ret=0;
        /* Rereserve */
        if(!VirtualAlloc(addr, PAGE_SIZE, MEM_RESERVE, PAGE_READWRITE)) ret=0;
      }
    }
    return ret;
  }
#endif
  return 0;
}
static size_t OSRemapMemoryPagesOntoAddrs(void *RESTRICT *addrs, size_t entries, PageFrameType *RESTRICT pageframes, OSAddressSpaceReservationData *RESTRICT data)
{
  if(!data->data[0])
  {
#ifdef ENABLE_PHYSICALPAGEEMULATION
    entries*=sizeof(PageFrameType)/sizeof(ULONG_PTR);
#endif
    return MapUserPhysicalPagesScatter(addrs, entries, (PULONG_PTR) pageframes);
  }
#ifdef ENABLE_PHYSICALPAGEEMULATION
  else
  {
    size_t n, ret=1;
    PageFrameType *RESTRICT pfa, *RESTRICT pf;
    for(n=0; n<entries; n++, addrs++, pageframes++)
    {
      if(*pageframe)
      {
        size_t filemappingoffset=PAGE_SIZE*((*pageframe)-1);
        /* Change reservation for next segment */
        if(!VirtualFree(*addrs, 0, MEM_RELEASE)) ret=0;
        if(!MapViewOfFileEx((HANDLE) data.data[0], FILE_MAP_ALL_ACCESS,
#if defined(_M_IA64) || defined(_M_X64) || defined(WIN64)
                           (DWORD)(filemappingoffset>>32),
#else
                           0,
#endif
                           (DWORD)(filemappingoffset & (DWORD)-1), PAGE_SIZE, *addrs)) ret=0;
      }
      else
      {
        if(!UnmapViewOfFile(*addrs)) ret=0;
        /* Rereserve */
        if(!VirtualAlloc(*addrs, PAGE_SIZE, MEM_RESERVE, PAGE_READWRITE)) ret=0;
      }
    }
    return ret;
  }
#endif
  return 0;
}

#endif

/* Maps an address reservation */
typedef struct FreePageHeader_t FreePageHeader;
struct FreePageHeader_t
{
  FreePageHeader *older, *newer;
};
typedef struct AddressSpaceReservation_s AddressSpaceReservation_t;
static struct AddressSpaceReservation_s
{
  OSAddressSpaceReservationData OSreservedata;
  AddressSpaceReservation_t *RESTRICT next;
  void *front, *frontptr;         /* Grows upward */
  void *back, *backptr;           /* Grows downward */
  FreePageHeader *oldestclean, *newestclean;
  FreePageHeader *oldestdirty, *newestdirty;
  size_t freepages;
  size_t usedpages;               /* Doesn't include pages used to store this structure */
  PageFrameType pagemapping[1];   /* Includes this structure */
} *RESTRICT addressspacereservation;

static AddressSpaceReservation_t *ReserveSpace(size_t space)
{
  const size_t RESERVEALWAYSLEAVEFREE=64*1024*1024; /* Windows goes seriously screwy if you take away all address space */
  OSAddressSpaceReservationData addrR;
  AddressSpaceReservation_t *RESTRICT addr=0;
  size_t pagemappingsize, n, pagesallocated;
  PageFrameType pagebuffer[256];
  if(space<(size_t)1<<30 /* 1Gb */)
  {
    space=(size_t)1<<30;
    if(8==sizeof(size_t)) space<<=2; /* Go for 4Gb chunks on 64 bit */
  }
  while(space>=RESERVEALWAYSLEAVEFREE && !(addrR=OSReserveAddrSpace(space)).addr)
    space>>=1;
  if(space<RESERVEALWAYSLEAVEFREE) return 0;
  pagemappingsize=sizeof(AddressSpaceReservation_t)+sizeof(PageFrameType)*((space/PAGE_SIZE)-2);
  pagemappingsize=(pagemappingsize+PAGE_SIZE-1) &~(PAGE_SIZE-1);
  pagemappingsize/=PAGE_SIZE;
  /* We now need pagemappingsize number of pages in order to store the mapping tables, but
  because this could be as much as 4Mb of stuff we'll need to do it in chunks to avoid
  breaking the stack. */
  for(n=0; n<pagemappingsize; n+=pagesallocated)
  {
    size_t torequest=sizeof(pagebuffer)/sizeof(PageFrameType);
    void *mapaddr=(void *)((size_t) addrR.addr + n*PAGE_SIZE);
    if(torequest>pagemappingsize-n) torequest=pagemappingsize-n;
    if(!(pagesallocated=OSObtainMemoryPages(pagebuffer, torequest, &addrR)))
      goto badexit;
    if(!OSRemapMemoryPagesOntoAddr(mapaddr, pagesallocated, pagebuffer, &addrR))
      goto badexit;
    if(!n)
    { /* This is the first run, so install AddressSpaceReservation */
      addr=(AddressSpaceReservation_t *RESTRICT) addrR.addr;
      addr->OSreservedata=addrR;
      addr->front=addr->frontptr=(void *)((size_t)addr+pagemappingsize);
      addr->back=addr->backptr=(void *)((size_t)addr+space);
    }
    /* Add these new pages to the page mappings. Because we are premapping in new pages,
    we are guaranteed to have memory already there ready for us. */
    for(torequest=0; torequest<pagesallocated; torequest++)
      addr->pagemapping[n+torequest]=pagebuffer[n];
  }
  return addr;
badexit:
  /* Firstly throw away any just allocated pages */
  if(pagesallocated)
    OSReleaseMemoryPages(pagebuffer, pagesallocated, &addrR);
  if(addr)
  { /* Now throw away any previously stored */
    OSReleaseMemoryPages(addr->pagemapping, n, &addrR);
  }
  OSReleaseAddrSpace(&addrR, space);
  return 0;
}
static int CheckFreeAddressSpaces(AddressSpaceReservation_t *RESTRICT *RESTRICT _addr)
{
  AddressSpaceReservation_t *RESTRICT addr=*_addr;
  if(!addr->next || CheckFreeAddressSpaces(&addr->next))
  {
    assert(!addr->next);
    if(0==addr->usedpages)
    {
      size_t size=(size_t)addr->back-(size_t)addr;
      assert(addr->frontptr==addr->front);
      assert(addr->backptr==addr->back);
      if(OSReleaseAddrSpace(&addr->OSreservedata, size))
      {
        *_addr=0;
        return 1;
      }
    }
  }
  return 0;
}
typedef struct RemapMemoryPagesBlock_t
{
    void *addrs[16];
    PageFrameType pageframes[16];
} RemapMemoryPagesBlock;
static PageFrameType *RESTRICT FillWithFreePages(AddressSpaceReservation_t *RESTRICT addr, PageFrameType *RESTRICT start, PageFrameType *RESTRICT end, int needclean)
{
  PageFrameType *RESTRICT pf;
  RemapMemoryPagesBlock memtodecommit;
  size_t memtodecommitidx=0;
  for(pf=start; pf!=end; pf++)
  {
    FreePageHeader *RESTRICT *RESTRICT freepage=0;
    int wipeall=0;
    if(addr->freepages)
    {
      if(needclean && addr->oldestclean)
      {
        assert(!addr->oldestclean->older);
        freepage=&addr->oldestclean;
      }
      else if(addr->oldestdirty)
      {
        assert(!addr->oldestdirty->older);
        freepage=&addr->oldestdirty;
        wipeall=1;
      }
      else if(!needclean && addr->oldestclean)
      {
        assert(!addr->oldestclean->older);
        freepage=&addr->oldestclean;
      }
      /* Add to the list of pages to demap */
      memtodecommit.addrs[memtodecommitidx]=*freepage;
      memtodecommit.pageframes[memtodecommitidx]=0;
      /* Remove from free page lists */
      assert(!(*freepage)->older);
      *freepage=(*freepage)->newer;
      if(*freepage)
        (*freepage)->older=0;
      addr->freepages--;
      addr->usedpages++;
      if(needclean)
      {
        if(wipeall)
          memset(memtodecommit.addrs[memtodecommitidx], 0, PAGE_SIZE);
        else
          memset(memtodecommit.addrs[memtodecommitidx], 0, sizeof(FreePageHeader));
      }
    }
    if(16==++memtodecommitidx || !freepage)
    {
      size_t n;
      /* Relocate these pages */
      for(n=0; n<memtodecommitidx; n++)
      {
        PageFrameType freepageframe;
        size_t freepagepfidx=((size_t)memtodecommit.addrs[n]-(size_t)addr)/PAGE_SIZE;
        freepageframe=addr->pagemapping[freepagepfidx];
        *(pf-memtodecommitidx+n)=freepageframe;
      }
      if(!OSRemapMemoryPagesOntoAddrs(memtodecommit.addrs, memtodecommitidx, memtodecommit.pageframes, &addr->OSreservedata))
        return pf-memtodecommitidx;
      memtodecommitidx=0;
      if(!freepage) break;
    }
  }
  /* Allocate more pages if needed */
  if(pf!=end)
  {
    size_t newpagesrequired=end-pf, newpagesobtained;
    newpagesobtained=OSObtainMemoryPages(pf, newpagesrequired, &addr->OSreservedata);
    if(newpagesrequired!=newpagesobtained)
    {
      if(newpagesobtained) OSReleaseMemoryPages(pf, newpagesobtained, &addr->OSreservedata);
      return pf-memtodecommitidx;
    }
    addr->usedpages+=newpagesobtained;
  }
  return pf;
}
static int ReleasePages(void *mem, size_t size);
static void *AllocatePages(void *mem, size_t size, unsigned flags)
{
  AddressSpaceReservation_t *RESTRICT addr;
  if(!addressspacereservation && !(addressspacereservation=ReserveSpace(0)))
  {
    fprintf(stderr, "User Mode Page Allocator: Failed to allocate initial address space\n");
    abort();
  }
  for(addr=addressspacereservation; addr; addr=addr->next)
  {
    int fromback=(flags & FLAG_TOPDOWN);
    if((mem && ((mem>=addr->front && mem<addr->frontptr && !(fromback=0)) || (mem>=addr->backptr && mem<addr->back && (fromback=1)))
      || (!mem && (size_t) addr->backptr - (size_t) addr->frontptr>=size)))
    {
      size_t n, sizeinpages=size/PAGE_SIZE;
      void *ret=mem ? mem : ((fromback) ? (void *)((size_t) addr->backptr - size) : addr->frontptr), *retptr;
      PageFrameType *RESTRICT pagemappingsbase=addr->pagemapping+((size_t)ret-(size_t)addr)/PAGE_SIZE, *RESTRICT pagemappings;
      int needtofillwithfree=0;
      if(!mem)
      {
        if(fromback)
          addr->backptr=(void *)((size_t) addr->backptr - size);
        else
          addr->frontptr=(void *)((size_t) addr->frontptr + size);
      }
      if(!(flags & FLAG_NOCOMMIT))
      { /* We leave memory still held by the application mapped at the addresses it was mapped at
        when freed and only nobble these when we need new pages. Hence between addresses ret and
        ret+size there may be a patchwork of already allocated regions, so what we do is to firstly
        delink any already mapped pages from the free page list and then to batch the filling in of
        the blank spots sixteen at a time. */
        pagemappings=pagemappingsbase;
        retptr=ret;
        for(n=0; n<sizeinpages; n++, pagemappings++, retptr=(void *)((size_t)retptr + PAGE_SIZE))
        {
          if(*pagemappings)
          {
            FreePageHeader *RESTRICT freepage=(FreePageHeader *RESTRICT) retptr, *RESTRICT *RESTRICT prevnextaddr, *RESTRICT *RESTRICT nextprevaddr;
            if(freepage->older)
            {
              assert(freepage->older->newer==freepage);
              prevnextaddr=&freepage->older->newer;
            }
            else if(addr->oldestdirty==freepage)
            {
              assert(!freepage->older);
              prevnextaddr=&addr->oldestdirty;
            }
            else if(addr->oldestclean==freepage)
            {
              assert(!freepage->older);
              prevnextaddr=&addr->oldestclean;
            }
            if(freepage->newer)
            {
              assert(freepage->newer->older==freepage);
              nextprevaddr=&freepage->newer->older;
            }
            else if(addr->newestdirty==freepage)
            {
              assert(!freepage->newer);
              nextprevaddr=&addr->newestdirty;
            }
            else if(addr->newestclean==freepage)
            {
              assert(!freepage->newer);
              nextprevaddr=&addr->newestclean;
            }
            *prevnextaddr=freepage->newer;
            *nextprevaddr=freepage->older;
            addr->freepages--;
            addr->usedpages++;
            if(fromback)
              memset(freepage, 0, sizeof(FreePageHeader));
          }
          else
            needtofillwithfree=1;
        }
        if(needtofillwithfree)
        {
          pagemappings=pagemappingsbase;
          retptr=ret;
          for(n=0; n<sizeinpages; n++, pagemappings++, retptr=(void *)((size_t)retptr + PAGE_SIZE))
          {
            if(!*pagemappings)
            {
              PageFrameType *emptyframestart=pagemappings, *filledto;
              for(; n<sizeinpages && !*pagemappings; n++, pagemappings++, retptr=(void *)((size_t)retptr + PAGE_SIZE));
              if(pagemappings!=(filledto=FillWithFreePages(addr, emptyframestart, pagemappings, fromback)))
              {
                pagemappings=filledto;
                break;
              }
            }
          }
          if(!OSRemapMemoryPagesOntoAddr(ret, pagemappings-pagemappingsbase, pagemappingsbase, &addr->OSreservedata) || n<sizeinpages)
          { /* We failed to allocate everything, so release */
            ReleasePages(ret, size);
            return 0;
          }
        }
      }
      return ret;
    }
    if(!addr->next)
    {
      addr->next=ReserveSpace((size_t) 1<<(nedtriebitscanr(size-1)+1));
    }
  }
  return 0;
}
static AddressSpaceReservation_t *RESTRICT AddressSpaceFromMem(int *RESTRICT fromback, void *mem)
{
  AddressSpaceReservation_t *RESTRICT addr;
  for(addr=addressspacereservation; addr; addr=addr->next)
  {
    if(mem>=addr->front && mem<addr->back)
    {
      if(fromback) *fromback=mem>=addr->backptr;
      return addr;
    }
  }
  return 0;
}
static int ReleasePages(void *mem, size_t size)
{ /* Returns 1 if address space was freed */
  int fromback;
  AddressSpaceReservation_t *RESTRICT addr=AddressSpaceFromMem(&fromback, mem);
  if(addr)
  {
    FreePageHeader *RESTRICT freepage=(FreePageHeader *RESTRICT) mem;
    size_t n, sizeinpages=size/PAGE_SIZE;
    PageFrameType *RESTRICT pagemappings=addr->pagemapping+((size_t)freepage-(size_t)addr)/PAGE_SIZE;
    if(mem>=addr->frontptr && mem<addr->backptr)
    {
      fprintf(stderr, "User Mode Page Allocator: Attempt to free memory in dead man's land\n");
      assert(0);
      abort();
    }
    for(n=0; n<sizeinpages; n++, pagemappings++, freepage=(FreePageHeader *RESTRICT)((size_t)freepage + PAGE_SIZE))
    {
      if(*pagemappings)
      {
        freepage->older=addr->newestdirty;
        freepage->newer=0;
        if(addr->newestdirty)
        {
          assert(!addr->newestdirty->newer);
          addr->newestdirty->newer=freepage;
        }
        else
          addr->oldestdirty=freepage;
        addr->newestdirty=freepage;
        addr->freepages++;
        addr->usedpages--;
      }
    }
    if((size_t) addr->frontptr-size==(size_t) mem || (size_t) addr->backptr==(size_t) mem)
    {
      if(fromback)
        addr->backptr=(void *)((size_t) addr->backptr + size);
      else
        addr->frontptr=(void *)((size_t) addr->frontptr - size);
      //if(!addr->pagesused)
      //  CheckFreeAddressSpaces(&addressspacereservation);
      return 1;
    }
    return 0;
  }
  return 0;
}

#define REGIONSTORAGESIZE (PAGE_SIZE*4)
#define REGIONSPERSTORAGE ((REGIONSTORAGESIZE-2*sizeof(void *))/sizeof(region_node_t))
typedef struct RegionStorage_s RegionStorage_t;
static struct RegionStorage_s
{
  RegionStorage_t *next;
  region_node_t *freeregions;
  region_node_t regions[REGIONSPERSTORAGE];
} *regionstorage;
static size_t regionsallocated, regionsfree;

static region_node_t *AllocateRegionNode(void)
{
  int n;
  if(!regionstorage || !regionstorage->freeregions)
  {
    RegionStorage_t **_rs=!regionstorage ? &regionstorage : &regionstorage->next, *rs;
    if(regionstorage) while(*_rs) _rs=&((*_rs)->next);
    assert(sizeof(RegionStorage_t)<=REGIONSTORAGESIZE);
    if((rs=*_rs=(RegionStorage_t *) AllocatePages(0, REGIONSTORAGESIZE, FLAG_TOPDOWN)))
    {
      for(n=REGIONSPERSTORAGE-1; n>=0; n--)
      {
        *(region_node_t **)&rs->regions[n]=regionstorage->freeregions;
        regionstorage->freeregions=&rs->regions[n];
      }
      regionsallocated+=REGIONSPERSTORAGE;
      regionsfree+=REGIONSPERSTORAGE;
    }
  }
  if(regionstorage->freeregions)
  {
    region_node_t *ret=regionstorage->freeregions;
    regionstorage->freeregions=*(region_node_t **)ret;
    *(region_node_t **)ret=0;
    regionsfree--;
    return ret;
  }
  return 0;
}
static int CheckFreeRegionNodes(RegionStorage_t **_rs)
{
  if(regionsfree==regionsallocated)
  {
    if(!(*_rs)->next || CheckFreeRegionNodes(&(*_rs)->next))
    {
      assert(!(*_rs)->next);
      ReleasePages(*_rs, REGIONSTORAGESIZE);
      *_rs=0;
      regionsallocated=regionsfree=0;
      return 1;
    }
  }
  return 0;
}
static void FreeRegionNode(region_node_t *node)
{
  memset(node, 0, sizeof(region_node_t));
  *(region_node_t **)node=regionstorage->freeregions;
  regionstorage->freeregions=node;
  regionsfree++;
}

#if USE_LOCKS
static MLOCK_T userpagemutex;
#endif

static void *userpage_malloc(size_t toallocate, unsigned flags)
{
  void *ret=0;
  region_node_t node, *r;
  MemorySource *source=(flags & M2_RESERVED1) ? &upper : &lower;
  unsigned mremapvalue = (flags & M2_RESERVE_MASK)>>8;
  size_t size = mremapvalue ? ((flags & M2_RESERVE_ISMULTIPLIER) ? toallocate*mremapvalue : (size_t)1<<mremapvalue) : toallocate;
  if(size < toallocate)
    size = toallocate;
  /* Firstly find out if there is a free slot of sufficient size and if so use that.
  If there isn't a sufficient free slot, extend the virtual address space */
  node.start=0;
  node.end=(void *)size;
#if USE_LOCKS
  ACQUIRE_LOCK(&userpagemutex);
#endif
  r=REGION_NFIND(regionL_tree_s, &source->regiontreeL, &node);
  if(r)
  {
    size_t rlen=(size_t) r->end - (size_t) r->start;
    if(rlen<size)
    {
      assert(rlen>=size);
      abort();
    }
    REGION_REMOVE(regionS_tree_s, &source->regiontreeS, r);
    REGION_REMOVE(regionL_tree_s, &source->regiontreeL, r);
    if(rlen==size)
    { /* Release the node entirely */
      ret=r->start;
      REGION_REMOVE(regionE_tree_s, &source->regiontreeE, r);
      FreeRegionNode(r);
      goto commitpages;
    }
    /* Reinsert r with new start addr */
    ret=r->start;
    r->start=(void *)((size_t) r->start + size);
    REGION_INSERT(regionS_tree_s, &source->regiontreeS, r);
    REGION_INSERT(regionL_tree_s, &source->regiontreeL, r);
  }
  else
  { /* Reserve sufficient new address space */
    ret=AllocatePages(0, size, FLAG_NOCOMMIT|(source==&upper ? FLAG_TOPDOWN : 0));
  }
commitpages:
  if(!ret) goto mfail;
  if(!AllocatePages(ret, toallocate, 0))
  {
    ReleasePages(ret, size);
    goto mfail;
  }
#if USE_LOCKS
  RELEASE_LOCK(&userpagemutex);
#endif
  return ret;
mfail:
#if USE_LOCKS
  RELEASE_LOCK(&userpagemutex);
#endif
  return MFAIL;
}

static int userpage_free(void *mem, size_t size)
{
  region_node_t node, *RESTRICT prev, *RESTRICT next, *RESTRICT r=0;
  int fromback;
  MemorySource *source=0;
  /* Can I merge with adjacent free blocks? */
  node.start=(void *)((size_t)mem+size);
  node.end=mem;
#if USE_LOCKS
  ACQUIRE_LOCK(&userpagemutex);
#endif
  if(AddressSpaceFromMem(&fromback, mem))
    source=fromback ? &upper : &lower;
  else
    goto fail;
  prev=REGION_FIND(regionE_tree_s, &source->regiontreeE, &node);
  next=REGION_FIND(regionS_tree_s, &source->regiontreeS, &node);
  node.start=mem;
  node.end=(void *)((size_t)mem+size);
  if(prev && next)
  { /* Consolidate into prev */
    assert(prev->end==node.start);
    assert(next->start==node.end);
    REGION_REMOVE(regionS_tree_s, &source->regiontreeS, next);
    REGION_REMOVE(regionE_tree_s, &source->regiontreeE, next);
    REGION_REMOVE(regionL_tree_s, &source->regiontreeL, next);
    node.end=next->end;
    FreeRegionNode(next);
  }
  if(prev)
  { /* Consolidate into prev */
    assert(prev->end==node.start);
    REGION_REMOVE(regionE_tree_s, &source->regiontreeE, prev);
    REGION_REMOVE(regionL_tree_s, &source->regiontreeL, prev);
    r=prev;
    r->end=node.end;
  }
  else if(next)
  { /* Consolidate into next */
    assert(next->start==node.end);
    REGION_REMOVE(regionS_tree_s, &source->regiontreeS, next);
    REGION_REMOVE(regionL_tree_s, &source->regiontreeL, next);
    r=next;
    r->start=node.start;
  }
  if(ReleasePages(r ? r->start : node.start, r ? (size_t)r->end - (size_t)r->start : (size_t)node.end - (size_t)node.start))
  {
    if(prev)
    {
      assert(r==prev);
      REGION_REMOVE(regionS_tree_s, &source->regiontreeS, prev);
    }
    else if(next)
    {
      assert(r==next);
      REGION_REMOVE(regionE_tree_s, &source->regiontreeE, next);
    }
    if(r) FreeRegionNode(r);
  }
  else
  {
    if(!r)
    {
      r=AllocateRegionNode();
      assert(r);
      if(r)
      {
        r->start=node.start;
        r->end=node.end;
      }
      else
        goto fail;
    }
    if(r)
    {
      if(r!=prev)
      {
        REGION_INSERT(regionS_tree_s, &source->regiontreeS, r);
      }
      if(r!=next)
      {
        REGION_INSERT(regionE_tree_s, &source->regiontreeE, r);
      }
      REGION_INSERT(regionL_tree_s, &source->regiontreeL, r);
    }
  }
  assert(NEDTRIE_COUNT(&source->regiontreeS)==NEDTRIE_COUNT(&source->regiontreeE));
  assert(NEDTRIE_COUNT(&source->regiontreeS)==NEDTRIE_COUNT(&source->regiontreeL));
#if USE_LOCKS
  RELEASE_LOCK(&userpagemutex);
#endif
  return 0;
fail:
#if USE_LOCKS
  RELEASE_LOCK(&userpagemutex);
#endif
  return -1;
}

static void *userpage_realloc(void *mem, size_t oldsize, size_t newsize, int flags, unsigned flags2)
{
	return MFAIL;
}

#endif
