/*
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <cstdlib>
#include <driver/misc/models/cernRoot/rootUtil.h>
#include <boost/filesystem.hpp>
#include "TList.h"
#include "TFile.h"

using namespace std;
using namespace ::driver::misc;
using namespace chaos::metadata_service_client;
#include <boost/thread.hpp>
#include <math.h>
static int tot_error = 0;
static int exit_after_nerror = 1;
/*
 *
 */
chaos::common::data::CDWShrdPtr file2CD(boost::filesystem::path& p){
   if(boost::filesystem::is_regular_file(p)){
    std::string name=p.string();
    ifstream infile(name, std::ofstream::binary);
    infile.seekg(0, infile.end);
    long size = infile.tellg();
    infile.seekg(0);
    char* buffer = new char[size];
    infile.read(buffer, size);
    infile.close();
    try{
    chaos::common::data::CDWShrdPtr new_obj(new chaos::common::data::CDataWrapper((const char*)buffer, size));
    delete [] buffer;
    return new_obj;

    } catch(chaos::CException& e){
       delete [] buffer;


     return 0;
  }

   }
  return chaos::common::data::CDWShrdPtr();
}

TTree* file2Tree(boost::filesystem::path& p){
  
  
    chaos::common::data::CDWShrdPtr ret=file2CD(p);
    if(ret.get()==NULL){
      return NULL;
    }
    std::string name=p.string();

            //  DBG << "retriving \"" << *it << "\" seq:" << iseq << " runid:" << irunid << " data size:" << size;
    return getTreeFromCDataWrapper(*ret.get(),name,name);

}


struct path_leaf_string
{
    std::string operator()(const boost::filesystem::directory_entry& entry) const
    {
        return entry.path().leaf().string();
    }
};
int main(int argc, const char **argv) {


  if(argc<2){
    cout<<"## You must provide a file or directory, Usage: <dir/file> [treename]"<<std::endl;
    return -1;
  }
  std::string treename=argv[1];
  if(argc==3){
    treename=argv[2];
  }
  boost::filesystem::path p(argv[1]);
  if(boost::filesystem::is_regular_file(p)){
    TTree* ret=file2Tree(p);

    if(ret){
      //std::string dir=p.parent().string();
      std::string tname=treename+std::string(".root");
      TFile*tdir=new TFile(tname.c_str(),"RECREATE");

      //ret->SetDirectory(tdir);
      ret->Write();
      tdir->Close();
      delete ret;
      delete tdir;
      return 0;
    }
    cerr<<"## cannot convert file:"<<p<<std::endl;
    return -1;


  } else if(boost::filesystem::is_directory(p)){
    int count=0;
    std::vector<boost::filesystem::path> fils;
    std::string name=treename+".root";
    TFile* fout = new TFile(name.c_str(),"RECREATE");


   // std::transform(boost::filesystem::directory_iterator(p), boost::filesystem::directory_iterator(), std::back_inserter(fils));

    std::copy(boost::filesystem::directory_iterator(p), boost::filesystem::directory_iterator(), std::back_inserter(fils));
    std::sort(fils.begin(), fils.end());
   // TList *list = new TList; 
    ChaosToTree ti(name);
    for(std::vector<boost::filesystem::path>::iterator i=fils.begin();(i!=fils.end())/*&&(count<100)*/;i++){
      cout<<"converting "<<*i<<" count:"<<count++<<std::endl;
      chaos::common::data::CDWShrdPtr cd=file2CD(*i);
      if(cd.get()){
        ti.addData(*cd.get());
      }
      
    }
  
    if(count){
      TTree *newtree = ti.getTree();
      fout->Write();

    
      cout<<"converted:"<<count<<" files into:"<<name<<std::endl;
      fout->Close();
}
      return 0;
    
    }

      cerr<<"## no file or directory specified"<<std::endl;

  }
