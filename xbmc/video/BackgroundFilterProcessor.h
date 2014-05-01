#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "commons/Exception.h"
#include "playlists/SmartPlayList.h"
#include "utils/LabelFormatter.h"
#include "utils/JobManager.h"
#include "utils/SortUtils.h"
#include "view/GUIViewState.h"
#include "FileItem.h"

class FilterProcessorInterruptException : public XbmcCommons::Exception {
public:
  inline FilterProcessorInterruptException(const FilterProcessorInterruptException& other) : Exception(other) { }
  inline FilterProcessorInterruptException() : Exception("FilterProcessorInterruptException") { }
};

class CBackgroundFilterMessageSenderJob : public CJob {
public:
  CBackgroundFilterMessageSenderJob(const CFileItemList& items, const int selectedDBId);
  virtual ~CBackgroundFilterMessageSenderJob();

  virtual bool DoWork();
  virtual const char* GetType() const {
    return "FilterMessage";
  }

  virtual bool operator==(const CJob* job) const;

  CFileItemList m_itemList;
  int m_selectedDBId;
  int m_jobId;

  static int s_jobId;
};

class CBackgroundFilterProcessor : public IRunnable, public CJobQueue {
public:
  CBackgroundFilterProcessor();
  virtual ~CBackgroundFilterProcessor();

  virtual void Run();

  bool IsFiltering();
  void StartFiltering(
      const CStdString &filter, const CStdString &filterPath, const CFileItemList& items,
      const CGUIViewState* pViewState, const CSmartPlaylist& advancedFilter, const int selectedDBId, bool canFilterAdvanced);
  void StopFiltering();

protected:
  CStdString RemoveParameterFromPath(const CStdString &strDirectory, const CStdString &strParameter);

  bool GetFilteredItems(const CStdString &filter, CFileItemList &items);
  bool GetAdvanceFilteredItems(CFileItemList &items);
  bool ApplyWatchedFilter(CFileItemList &items);

  void FormatItemLabels(CFileItemList &items, const LABEL_MASKS &labelMasks);
  void FormatAndSort(CFileItemList &items);
  void GetGroupedItems(CFileItemList &items);

  void CheckThreadInterrupted();

  CFileItemList m_itemList;
  CSmartPlaylist m_advancedFilter;
  CStdString m_strFilter;
  CStdString m_strFilterPath;

  LABEL_MASKS m_viewStateLabelMasks;
  SortDescription m_viewStateSortMethod;
  SortOrder m_viewStateSortOrder;

  int m_selectedDBId;
  bool m_canFilterAdvanced;

  bool m_bStop;
  bool m_bIsFiltering;

  CCriticalSection m_lock;
  CThread *m_thread;
};

