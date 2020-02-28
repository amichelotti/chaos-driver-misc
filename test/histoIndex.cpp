/*
 * File:   testDataSetAttribute.cpp
 * Author: michelo
 * test from UI
 * Created on September 10, 2015, 11:24 AM
 */

#include <boost/filesystem.hpp>
#include <cstdlib>

using namespace std;
#include <chaos/common/data/CDataWrapper.h>
#include <chaos/common/utility/TimingUtil.h>
#include <math.h>
static int tot_error = 0;
static int exit_after_nerror = 1;
/*
 *
 */
void histo2Test(boost::filesystem::path &p, const std::string &treename) {
  if (boost::filesystem::is_regular_file(p)) {
    bson_error_t src_err;

    bson_reader_t *src =
        bson_reader_new_from_file(p.string().c_str(), &src_err);
    const bson_t *b;
    bool eof;
    uint32_t countele = 0;
    uint64_t ts =
        chaos::common::utility::TimingUtil::getTimeStampInMicroseconds();
    std::string name = treename + ".txt";
    int64_t oldseq = -1;
    int64_t oldrunid = -1;
    int64_t oldtime = -1;
    ofstream fout(name);
    uint32_t document_len = 0;
    const uint8_t *document = NULL;
    fout << "count,mdsts,runid,seqid,hwts,size" << std::endl;

    while (((b = bson_reader_read(src, &eof)) != NULL) && (eof == false)) {
      bson_iter_t iter;
      if (bson_iter_init(&iter, b) == false) {
        cout << countele << " cannot init iterator" << std::endl;
        fout << countele << ",--"
             << ",--"
             << ",--"
             << ",--,--,--" << std::endl;

        countele++;
        continue;
      }
      if (bson_iter_next(&iter) == false) {
        cout << countele << " cannot init next iterator" << std::endl;
        fout << countele << ",--"
             << ",--"
             << ",--"
             << ",--,--" << std::endl;
        countele++;
        continue;
      }

      bson_iter_document(&iter, &document_len, &document);
      chaos::common::data::CDataWrapper cd((const char *)document,
                                           document_len);
      uint64_t iseq, irunid, hwts;
      int64_t tim;

      iseq = cd.getInt64Value(chaos::DataPackCommonKey::DPCK_SEQ_ID);
      irunid = cd.getInt64Value(chaos::ControlUnitDatapackCommonKey::RUN_ID);
      tim = cd.getInt64Value(
          chaos::NodeHealtDefinitionKey::NODE_HEALT_MDS_TIMESTAMP);
      hwts = cd.getInt64Value(chaos::DataPackCommonKey::DPCK_TIMESTAMP);
      // bson_destroy(bsona);
      if (oldrunid < 0) {
        oldrunid = irunid;
      }

      if (oldseq < 0) {
        oldseq = iseq;
      } else {
        if ((oldrunid == irunid) && (oldseq + 1) != iseq) {
          cout << countele << ",s:" << iseq << ",t:" << tim
               << "] seq discontinuity old:" << oldseq << " new:" << iseq
               << std::endl;
        }
      }
      if (oldrunid != irunid) {
        cout << countele << ",s:" << iseq << ",t:" << tim
             << "] change in run id, old:" << oldrunid << " new:" << irunid
             << std::endl;
        oldrunid = irunid;
      }

      if (oldrunid != irunid) {
        if (oldrunid > irunid) {
          cout << countele << ",s:" << iseq << ",t:" << tim
               << "] RUN ID INVERSION!! id, old:" << oldrunid
               << " new:" << irunid << std::endl;

        } else {
          cout << countele << ",s:" << iseq << ",t:" << tim
               << "] change in run id, old:" << oldrunid << " new:" << irunid
               << std::endl;
        }
        oldrunid = irunid;
      }
      if (oldtime > tim) {
        cout << countele << ",s:" << iseq << ",t:" << tim
             << "] TIME INVERSION!! id, old:" << oldtime << " new:" << tim
             << std::endl;
        oldrunid = irunid;
      }
      oldtime = tim;
      oldseq = iseq;
      countele++;
      fout << countele << "," << tim << "," << irunid << "," << iseq << ","
           << hwts << ","<<cd.getBSONRawSize()<<std::endl;
    }

    if (countele > 0) {
      uint64_t tt =
          chaos::common::utility::TimingUtil::getTimeStampInMicroseconds() - ts;

      cout << "converted:" << countele << " items in " << 1.0 * tt / 1000
           << " ms " << (1.0 * countele * 1000000 / tt)
           << " item/s into:" << name << std::endl;
    }
    fout.close();
  }
}

int main(int argc, const char **argv) {

  if (argc < 2) {
    cout << "## You must provide a BSON histo file: <file> [dumpname]"
         << std::endl;
    return -1;
  }
  std::string treename = argv[1];
  if (argc == 3) {
    treename = argv[2];
  }
  boost::filesystem::path p(argv[1]);
  if (boost::filesystem::is_regular_file(p)) {

    std::string name = treename + ".csv";
    histo2Test(p, treename);
    return 0;
  }
  cerr << "## not valid file specified" << std::endl;
  return 0;
}
