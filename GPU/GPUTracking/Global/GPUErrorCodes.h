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

/// \file GPUErrorCodes.h
/// \author David Rohr

// Error Codes for GPU Tracker
GPUCA_ERROR_CODE(0, ERROR_NONE)
GPUCA_ERROR_CODE(1, ERROR_ROWSTARTHIT_OVERFLOW)
GPUCA_ERROR_CODE(2, ERROR_STARTHIT_OVERFLOW)
GPUCA_ERROR_CODE(3, ERROR_TRACKLET_OVERFLOW)
GPUCA_ERROR_CODE(4, ERROR_TRACKLET_HIT_OVERFLOW)
GPUCA_ERROR_CODE(5, ERROR_TRACK_OVERFLOW)
GPUCA_ERROR_CODE(6, ERROR_TRACK_HIT_OVERFLOW)
GPUCA_ERROR_CODE(7, ERROR_GLOBAL_TRACKING_TRACK_OVERFLOW)
GPUCA_ERROR_CODE(8, ERROR_GLOBAL_TRACKING_TRACK_HIT_OVERFLOW)
GPUCA_ERROR_CODE(9, ERROR_LOOPER_OVERFLOW)
GPUCA_ERROR_CODE(10, ERROR_MERGER_CE_HIT_OVERFLOW)
GPUCA_ERROR_CODE(11, ERROR_MERGER_LOOPER_OVERFLOW)
GPUCA_ERROR_CODE(12, ERROR_SLICEDATA_FIRSTHITINBIN_OVERFLOW)
GPUCA_ERROR_CODE(13, ERROR_SLICEDATA_HITINROW_OVERFLOW)
GPUCA_ERROR_CODE(14, ERROR_SLICEDATA_BIN_OVERFLOW)
GPUCA_ERROR_CODE(15, ERROR_SLICEDATA_Z_OVERFLOW)
GPUCA_ERROR_CODE(16, ERROR_MERGER_HIT_OVERFLOW)
GPUCA_ERROR_CODE(17, ERROR_MERGER_TRACK_OVERFLOW)
GPUCA_ERROR_CODE(18, ERROR_COMPRESSION_ROW_HIT_OVERFLOW)
GPUCA_ERROR_CODE(19, ERROR_LOOPER_MATCH_OVERFLOW)
GPUCA_ERROR_CODE(20, ERROR_CF_PEAK_OVERFLOW)
GPUCA_ERROR_CODE(21, ERROR_CF_CLUSTER_OVERFLOW)
GPUCA_ERROR_CODE(22, ERROR_CF_ROW_CLUSTER_OVERFLOW)
GPUCA_ERROR_CODE(23, ERROR_CF_GLOBAL_CLUSTER_OVERFLOW)
GPUCA_ERROR_CODE(24, MAX_OVERFLOW_ERROR_NUMBER) // Overflow errors are detected as errno <= MAX_OVERFLOW_ERROR_NUMBER
GPUCA_ERROR_CODE(25, ERROR_TPCZS_INVALID_ROW)
GPUCA_ERROR_CODE(26, ERROR_TPCZS_INVALID_NADC)
GPUCA_ERROR_CODE(27, ERROR_TPCZS_INCOMPLETE_HBF)

// #define GPUCA_CHECK_TPCZS_CORRUPTION
