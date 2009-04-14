// Memory.cc -- Apr 14, 2009
//    by geohot
//  released under GPLv3, see http://gplv3.fsf.org/

#include "../include/macros.h"
#include "Memory.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace eda;

File& Memory::operator[](Data address)
{
  //info << "got access at " << address << ": ";

  std::map<int, std::vector<File> >::iterator addr;
  addr=--mChunks.upper_bound(address);
  if( address < (addr->first+addr->second.size()) )
  {
    //exists in chunk
    //info << "found in chunk" << std::endl;
    return addr->second[address-(addr->first)];
  }
  else
  {
    //return the random
    //info << "not found" << std::endl;
    return mMemoryUndefined[address];
  }
}

std::vector<File>* Memory::getChunk(Data address)
{
  std::map<int, std::vector<File> >::iterator addr;
  addr=mChunks.find(address);
  if(addr!=mChunks.end())
    return &(addr->second);
  else
    return 0;   //not found
}

//allocate a chunk of memory in mMemory
//we really should migrate this space from undefined
bool Memory::allocate(Data address, int len)
{
  info << "allocating region " << address << "-" << len << std::endl;
  if(checkRegion(address,len)) return false;
  std::vector<File> *region = new std::vector<File>;
  region->resize(len);
  mChunks.insert(std::make_pair(address,*region));
  return true;
}

//load a file into memory
//is it bad i only know how to do this in c?
bool Memory::loadFile(const char *filename, Data address)
{
  FILE *f=fopen(filename, "rb");
  if(f==0)
  {
    debug << "file " << filename << " not found" << std::endl;
    return false;
  }
  int len=fileSize(f);
  if(!allocate(address,len))
  {
    debug << "allocate failed" << std::endl;
    return false;
  }
  Data buffer[0x200];
  int rlen,cl=0;
  std::vector<File> *d=getChunk(address);
  if(d==0)
  {
    debug << "finding chunk failed" << std::endl;
    return false;
  }
//no byte stop files
  while((rlen=fread(buffer, 4, 0x200, f))>0)
  {
    for(int t=0;t<rlen;t++)
    {
      (*d)[(t*4)+cl].set(0,buffer[t]);      //loads are all on changelist 0 for now
    }
    cl+=rlen;
  }

  return true;
}

//see if a region is empty, ignore undefineds
//false is no region found
bool Memory::checkRegion(Data address, int len)
{
  std::map<int, std::vector<File> >::iterator addr;
  //get first start segment before (address+len)
  addr=mChunks.upper_bound(address+len);
  if(addr==mChunks.begin()) return false;
  addr--;
  //if ending in region or past region
  return ( address < (addr->first+addr->second.size()) );
}

void Memory::consoleDump(Data address, int len, int changelistNumber)
{
  for(int t=0; t<len; t+=4)
  {
    if(t!=0&&((t%0x10)==0)) printf("\n");
    std::cout << std::hex << std::setfill('0') << std::setw(8) <<
      (*this)[address+t][changelistNumber] << " ";
  }
  std::cout << std::endl;
}

void Memory::debugPrint()
{
  std::map<int, std::vector<File> >::iterator addr=mChunks.begin();
  while(addr!=mChunks.end())
  {
    std::cout << std::hex << "Region: 0x" << addr->first <<
      " - 0x" << addr->first+addr->second.size() << std::endl;
    ++addr;
  }
  std::cout << "plus " << std::dec << mMemoryUndefined.size() << " undefineds" << std::endl;
}

int Memory::fileSize(FILE *f)
{
  int pos,end;
  pos=ftell(f);
  fseek(f, 0, SEEK_END);
  end=ftell(f);
  fseek(f, pos, SEEK_SET);
  return end;
}
