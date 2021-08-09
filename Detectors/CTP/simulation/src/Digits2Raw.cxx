// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Digits2Raw.cxx
/// \author Roman Lietava

#include "CTPSimulation/Digits2Raw.h"
#include "FairLogger.h"
#include "CommonUtils/StringUtils.h"

using namespace o2::ctp;

/// CTP digits (see Digits.h) are inputs from trigger detectors.
/// GBT digit is temporary item: 80 bits long = 12 bits of BCid and payload (see digit2GBTdigit)
/// CTP digit -> GBTdigit CTPInt Record, GBTdigit TrigClass Record
/// GBT didit X -> CRU  raw data
/// X= CTPInt Record to link 0
/// X= TrigClass Record to link 1

void Digits2Raw::init()
{

  //
  // Register links
  //
  std::string outd = mOutDir;
  if (outd.back() != '/') {
    outd += '/';
  }
  LOG(INFO) << "Raw outpud dir:" << mOutDir;
  for (int ilink = 0; ilink < mNLinks; ilink++) {
    mFeeID = uint64_t(ilink);
    std::string outFileLink = mOutputPerLink ? o2::utils::Str::concat_string(outd, "ctp_link", std::to_string(ilink), ".raw") : o2::utils::Str::concat_string(outd, "ctp.raw");
    mWriter.registerLink(mFeeID, mCruID, ilink, mEndPointID, outFileLink);
  }
  mWriter.setEmptyPageCallBack(this);
}
void Digits2Raw::processDigits(const std::string& fileDigitsName)
{
  std::unique_ptr<TFile> digiFile(TFile::Open(fileDigitsName.c_str()));
  if (!digiFile || digiFile->IsZombie()) {
    LOG(FATAL) << "Failed to open input digits file " << fileDigitsName;
    return;
  }
  LOG(INFO) << "Processing digits to raw";
  TTree* digiTree = (TTree*)digiFile->Get("o2sim");
  if (!digiTree) {
    LOG(FATAL) << "Failed to get digits tree";
    return;
  }
  std::vector<o2::ctp::CTPDigit> CTPDigits, *fCTPDigitsPtr = &CTPDigits;
  if (digiTree->GetBranch("CTPDigits")) {
    digiTree->SetBranchAddress("CTPDigits", &fCTPDigitsPtr);
  } else {
    LOG(FATAL) << "Branch CTPDigits is missing";
    return;
  }
  o2::InteractionRecord intRec = {0, 0};
  // Get first orbit
  uint32_t orbit0 = 0;
  bool firstorbit = 1;
  // Add all CTPdigits for given orbit
  LOG(INFO) << "Number of entries: " << digiTree->GetEntries();
  for (int ient = 0; ient < digiTree->GetEntries(); ient++) {
    digiTree->GetEntry(ient);
    int nbc = CTPDigits.size();
    LOG(INFO) << "Entry " << ient << " : " << nbc << " BCs stored";
    std::vector<std::bitset<NGBT>> hbfIR;
    std::vector<std::bitset<NGBT>> hbfTC;
    for (auto const& ctpdig : CTPDigits) {
      std::cout << ctpdig.intRecord.orbit << " orbit bc " << ctpdig.intRecord.bc << std::endl;
      if ((orbit0 == ctpdig.intRecord.orbit) || firstorbit) {
        if (firstorbit == true) {
          firstorbit = false;
          orbit0 = ctpdig.intRecord.orbit;
          LOG(INFO) << "First orbit:" << orbit0;
        }
        std::bitset<NGBT> gbtdigIR;
        std::bitset<NGBT> gbtdigTC;
        digit2GBTdigit(gbtdigIR, gbtdigTC, ctpdig);
        std::cout << "ir:" << gbtdigIR << std::endl;
        hbfIR.push_back(gbtdigIR);
        hbfTC.push_back(gbtdigTC);
      } else {
        std::vector<char> buffer;
        //std::string PLTrailer;
        //std::memcpy(buffer.data() + buffer.size() - o2::raw::RDHUtils::GBTWord, PLTrailer.c_str(), o2::raw::RDHUtils::GBTWord);
        //addData for IR
        intRec.orbit = orbit0;
        if (mZeroSuppressedIntRec == true) {
          buffer = digits2HBTPayload(hbfIR, NIntRecPayload);
        } else {
          std::vector<std::bitset<NGBT>> hbfIRnonZS = addEmptyBC(hbfIR);
          buffer = digits2HBTPayload(hbfIRnonZS, NIntRecPayload);
        }
        std::cout << "buffer:" << buffer.size() << ":";
        for(auto const& c : buffer) std::cout << c ;
        std::cout << std::endl;
        mWriter.addData(CRULinkIDIntRec, mCruID, CRULinkIDIntRec, mEndPointID, intRec, buffer);
        // add data for Trigger Class Record
        buffer.clear();
        mWriter.addData(CRULinkIDClassRec, mCruID, CRULinkIDClassRec, mEndPointID, intRec, buffer);
        //
        orbit0 = ctpdig.intRecord.orbit;
        hbfIR.clear();
        hbfTC.clear();
      }
    }
  }
}
void Digits2Raw::emptyHBFMethod(const header::RDHAny* rdh, std::vector<char>& toAdd) const
{
  // TriClassRecord data zero suppressed
  // CTP INteraction Data
  if (o2::raw::RDHUtils::getCRUID(rdh) == CRULinkIDIntRec) {
    if (mZeroSuppressedIntRec == false) {
      toAdd.clear();
      std::vector<std::bitset<NGBT>> digits;
      for (uint32_t i = 0; i < o2::constants::lhc::LHCMaxBunches; i++) {
        std::bitset<NGBT> dig = i;
        digits.push_back(dig);
      }
      toAdd = digits2HBTPayload(digits, NIntRecPayload);
    }
  }
}
std::vector<char> Digits2Raw::digits2HBTPayload(const gsl::span<std::bitset<NGBT>> digits, uint32_t Npld) const
{
  std::vector<char> toAdd;
  uint32_t size_gbt = 0;
  std::bitset<NGBT> gbtword;
  // if not zero suppressed add (bcid,0)
  for (auto const& dig : digits) {
    std::bitset<NGBT> gbtsend;
    if (makeGBTWord(dig, gbtword, size_gbt, Npld, gbtsend) == true) {
      for (uint32_t i = 0; i < NGBT; i += 8) {
        uint32_t w = 0;
        for (uint32_t j = 0; j < 8; j++) {
          w += (1 << j) * gbtsend[i + j];
        }
        char c = w;
        toAdd.push_back(c);
      }
      // Pad zeros up to 128 bits
      uint32_t NZeros = (o2::raw::RDHUtils::GBTWord * 8 - NGBT) / 8;
      for (uint32_t i = 0; i < NZeros; i++) {
        char c = 0;
        toAdd.push_back(c);
      }
    }
  }
  return std::move(toAdd);
}
// Adding payload of size<NGBT to GBT words of size NGBT
// gbtsend valid only when return 1
bool Digits2Raw::makeGBTWord(const gbtword80_t& pld, gbtword80_t& gbtword, uint32_t& size_gbt, uint32_t Npld, gbtword80_t& gbtsend) const
{
  bool valid = false;
  //printBitset(gbtword,"GBTword");
  gbtword |= (pld << size_gbt);
  if ((size_gbt + Npld) < NGBT) {
    size_gbt += Npld;
  } else {
    // sendData
    //printBitset(gbtword,"Sending");
    gbtsend = gbtword;
    gbtword = pld >> (NGBT - size_gbt);
    size_gbt = size_gbt + Npld - NGBT;
    valid = true;
  }
  return valid;
}
int Digits2Raw::digit2GBTdigit(std::bitset<NGBT>& gbtdigitIR, std::bitset<NGBT>& gbtdigitTR, const CTPDigit& digit)
{
  //
  // CTP Interaction record (CTP inputs)
  //
  gbtdigitIR = 0;
  gbtdigitIR = (digit.CTPInputMask).to_ullong() << 12;
  gbtdigitIR |= digit.intRecord.bc;
  //
  // Trig Classes
  //
  gbtdigitTR = 0;
  gbtdigitTR = (digit.CTPClassMask).to_ullong() << 12;
  gbtdigitTR |= digit.intRecord.bc;
  return 0;
}
std::vector<std::bitset<NGBT>> Digits2Raw::addEmptyBC(std::vector<std::bitset<NGBT>>& hbfIRZS)
{
  std::vector<std::bitset<NGBT>> hbfIRnonZS;
  uint32_t bcnonzero = 0;
  for (auto const& item : hbfIRZS) {
    uint32_t bcnonzeroNext = (item.to_ulong()) & 0xfff;
    for (int i = (bcnonzero + 1); i < bcnonzeroNext; i++) {
      std::bitset<NGBT> bs = i;
      hbfIRnonZS.push_back(bs);
      bcnonzero = bcnonzeroNext;
    }
    hbfIRnonZS.push_back(item);
  }
  return hbfIRnonZS;
}
