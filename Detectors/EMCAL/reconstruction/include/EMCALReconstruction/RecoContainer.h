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

#include <cstdint>
#include <exception>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <gsl/span>
#include <Rtypes.h>
#include <CommonDataFormat/InteractionRecord.h>
#include <DataFormatsEMCAL/Cell.h>

namespace o2::emcal
{
/// \struct RecCellInfo
/// \brief Container class for cell information for merging
struct RecCellInfo {
  o2::emcal::Cell mCellData; ///< Cell information
  bool mIsLGnoHG;            ///< Cell has only LG digits
  bool mHGOutOfRange;        ///< Cell has only HG digits which are out of range
  int mFecID;                ///< FEC ID of the channel (for monitoring)
  int mDDLID;                ///< DDL of the channel (for monitoring)
  int mHWAddressLG;          ///< HW address of LG (for monitoring)
  int mHWAddressHG;          ///< HW address of HG (for monitoring)
};

/// \class EventContainer
/// \brief Containter of cells for a given event
/// \ingroup EMCALReconstruction
class EventContainer
{
 public:
  /// \brief Constructor
  EventContainer() = default;

  /// \brief Destructor
  ~EventContainer() = default;

  /// \brief Set trigger bits of the interaction
  /// \param triggerbits Trigger bits
  void setTriggerBits(uint64_t triggerbits) { mTriggerBits = triggerbits; }

  /// \brief Get trigger bits of the interaction
  /// \return Trigger bits
  uint64_t getTriggerBits() const { return mTriggerBits; }

  /// \brief Get cells in container
  /// \return List of cells in container
  const gsl::span<const RecCellInfo> getCells() const { return mCells; }

  /// \brief Get LEDMONs in container
  /// \return List of LEDMONs
  const gsl::span<const RecCellInfo> getLEDMons() const { return mLEDMons; }

  /// \brief Add cell information to the event container
  /// \param tower Tower ID
  /// \param energy Cell energy
  /// \param time Cell time
  /// \param celltype Cell type (high gain or low gain)
  /// \param hwaddress Hardware address
  /// \param fecID ID of the frontend card
  /// \param ddlID ID of the DDL
  /// \param doMergeHGLG If true merge with existing HG/LG cell
  ///
  /// In case of merge mode the priory is given to the HG digitizer (better resolution).
  /// As long as the energy is not in the saturation region (approx 16 GeV) the HG is selected,
  /// otherwise the LG digit is used.
  void setCell(int tower, double energy, double time, ChannelType_t celltype, int hwaddress, int fecID, int ddlID, bool doMergeHGLG)
  {
    setCellCommon(tower, energy, time, celltype, false, hwaddress, fecID, ddlID, doMergeHGLG);
  }

  /// \brief Add LEDMON information to the event container
  /// \param tower LEDMON ID
  /// \param energy LEDMON energy
  /// \param time LEDMON time
  /// \param celltype LEDMON type (high gain or low gain)
  /// \param hwaddress Hardware address
  /// \param fecID ID of the frontend card
  /// \param ddlID ID of the DDL
  /// \param doMergeHGLG If true merge with existing HG/LG LEDMON
  ///
  /// In case of merge mode the priory is given to the HG digitizer (better resolution).
  /// As long as the energy is not in the saturation region (approx 16 GeV) the HG is selected,
  /// otherwise the LG digit is used.
  void setLEDMONCell(int tower, double energy, double time, ChannelType_t celltype, int hwaddress, int fecID, int ddlID, bool doMergeHGLG)
  {
    setCellCommon(tower, energy, time, celltype, true, hwaddress, fecID, ddlID, doMergeHGLG);
  }

  /// \brief Sort Cells / LEDMONs in container according to tower / module ID
  /// \param isLEDmon Switch between Cell and LEDMON
  void sortCells(bool isLEDmon);

 private:
  /// \brief Common handler for adding cell/LEDMON information to the event container
  /// \param tower Tower / LEDMON ID
  /// \param energy Energy
  /// \param time Time
  /// \param celltype Digitizer type (high gain or low gain)
  /// \param hwaddress Hardware address
  /// \param fecID ID of the frontend card
  /// \param ddlID ID of the DDL
  /// \param doMergeHGLG Switch for merge mode
  void setCellCommon(int tower, double energy, double time, ChannelType_t celltype, bool isLEDmon, int hwaddress, int fecID, int ddlID, bool doMergeHGLG);

  /// \brief Check whether the energy is in the saturation limit
  /// \return True if the energy is in the saturation region, false otherwise
  bool isCellSaturated(double energy) const;

  uint64_t mTriggerBits;             ///< Trigger bits of the event
  std::vector<RecCellInfo> mCells;   ///< Container of cells in event
  std::vector<RecCellInfo> mLEDMons; ///< Container of LEDMONs in event
};

/// \class RecoContainer
/// \brief Handler for cells in
/// \ingroup EMCALReconstruction
class RecoContainer
{
 public:
  /// \class InteractionNotFoundException
  /// \brief Handling of access to trigger interaction record not present in container
  class InteractionNotFoundException : public std::exception
  {
   public:
    /// \brief Constructor
    /// \param currentIR Interaction record raising the exception
    InteractionNotFoundException(const o2::InteractionRecord& currentIR) : mCurrentIR(currentIR)
    {
      mMessage = "Interaction record not found: Orbit " + std::to_string(mCurrentIR.orbit) + ", BC " + std::to_string(mCurrentIR.bc);
    }

    /// \brief Destructor
    ~InteractionNotFoundException() noexcept final = default;

    /// \brief Get error message of the exception
    /// \return Error message
    const char* what() const noexcept final
    {
      return mMessage.data();
    };

    /// \brief Get interaction record raising the exception
    /// \return Interaction record
    const o2::InteractionRecord& getInteractionRecord() const { return mCurrentIR; }

   private:
    o2::InteractionRecord mCurrentIR; ///< Interaction record raising the exception
    std::string mMessage;             ///< Error message
  };

  /// \brief Constructor
  RecoContainer() = default;

  /// \brief Destructor
  ~RecoContainer() = default;

  /// \brief Get container for trigger
  /// \param currentIR Interaction record of the trigger
  /// \return Container for trigger (creating new container if not yet present)
  EventContainer& getEventContainer(const o2::InteractionRecord& currentIR);

  /// \brief Get container for trigger (read-only)
  /// \param currentIR Interaction record of the trigger
  /// \return Container for trigger
  /// \throw InteractionNotFoundException if interaction record is not present
  const EventContainer& getEventContainer(const o2::InteractionRecord& currentIR) const;

  /// \brief Get sorted vector interaction records of triggers in container
  /// \return Sorted vector of container
  std::vector<o2::InteractionRecord> getOrderedInteractions() const;

  /// \brief Get number of events in container
  /// \return Number of events
  std::size_t getNumberOfEvents() const { return mEvents.size(); }

  /// \brief Clear container
  void reset() { mEvents.clear(); }

 private:
  std::unordered_map<o2::InteractionRecord, EventContainer> mEvents; ///< Containers in event
};

class RecoContainerReader
{
 public:
  class InvalidAccessException : public std::exception
  {
   public:
    InvalidAccessException() = default;
    ~InvalidAccessException() noexcept final = default;

    const char* what() const noexcept final { return "Access to invalid element in reco container"; }
  };
  RecoContainerReader(const RecoContainer& container);
  RecoContainerReader(const RecoContainer&& container) = delete;

  ~RecoContainerReader() = default;

  const EventContainer& nextEvent();
  bool hasNext() const { return mCurrentEvent < mOrderedInteractions.size(); }
  std::size_t getNumberOfEvents() const { return mDataContainer.getNumberOfEvents(); }

 private:
  const RecoContainer& mDataContainer;
  std::vector<o2::InteractionRecord> mOrderedInteractions;
  std::size_t mCurrentEvent = 0;
};

} // namespace o2::emcal