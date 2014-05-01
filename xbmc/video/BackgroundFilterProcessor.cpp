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

#include "filesystem/SmartPlaylistDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/GroupUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "BackgroundFilterProcessor.h"
#include "GUIUserMessages.h"
#include "URL.h"

using namespace XFILE;
using namespace VIDEODATABASEDIRECTORY;
using namespace std;

#define PROPERTY_PATH_DB      "path.db"
#define PROPERTY_GROUP_BY     "group.by"
#define PROPERTY_GROUP_MIXED  "group.mixed"


int CBackgroundFilterMessageSenderJob::s_jobId = 0;

CBackgroundFilterMessageSenderJob::CBackgroundFilterMessageSenderJob(const CFileItemList& items, const int selectedDBId) {
  m_itemList.Copy(items, false);
  m_itemList.Append(items);
  m_selectedDBId = selectedDBId;
  m_jobId = ++s_jobId;
}

CBackgroundFilterMessageSenderJob::~CBackgroundFilterMessageSenderJob() {}

bool CBackgroundFilterMessageSenderJob::operator==(const CJob* job) const {
  if (strcmp(job->GetType(),GetType()) == 0) {
    const CBackgroundFilterMessageSenderJob* jobExtract = dynamic_cast<const CBackgroundFilterMessageSenderJob*>(job);
    if (jobExtract && jobExtract->m_jobId == m_jobId)
      return true;
  }
  return false;
}

bool CBackgroundFilterMessageSenderJob::DoWork() {
  CLog::Log(LOGDEBUG, "%s: ### %s", __FUNCTION__, "Preparing message");
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_FILTER_ITEMS_FILTERED, m_selectedDBId, &m_itemList);
  g_windowManager.SendThreadMessage(message);
  CLog::Log(LOGDEBUG, "%s: ### %s", __FUNCTION__, "Message sent");

  return true;
}

CBackgroundFilterProcessor::CBackgroundFilterProcessor() : m_thread (NULL), CJobQueue(true, 1, CJob::PRIORITY_HIGH) {
  m_selectedDBId = -1;
  m_bStop = false;
  m_bIsFiltering = false;
  m_canFilterAdvanced = false;
}

CBackgroundFilterProcessor::~CBackgroundFilterProcessor() {
  StopFiltering();
}

bool CBackgroundFilterProcessor::GetFilteredItems(const CStdString &filter, CFileItemList &items)
{
  bool result = false;
  if (m_canFilterAdvanced)
    result = GetAdvanceFilteredItems(items);

  CheckThreadInterrupted();

  CStdString trimmedFilter(filter);
  StringUtils::TrimLeft(trimmedFilter);
  StringUtils::ToLower(trimmedFilter);

  if (trimmedFilter.empty())
    return result;

  CFileItemList filteredItems(items.GetPath()); // use the original path - it'll likely be relied on for other things later.
  bool numericMatch = StringUtils::IsNaturalNumber(trimmedFilter);
  for (int i = 0; i < items.Size(); i++)
  {
    CheckThreadInterrupted();
    CFileItemPtr item = items.Get(i);
    if (item->IsParentFolder())
    {
      filteredItems.Add(item);
      continue;
    }
    // TODO: Need to update this to get all labels, ideally out of the displayed info (ie from m_layout and m_focusedLayout)
    // though that isn't practical.  Perhaps a better idea would be to just grab the info that we should filter on based on
    // where we are in the library tree.
    // Another idea is tying the filter string to the current level of the tree, so that going deeper disables the filter,
    // but it's re-enabled on the way back out.
    CStdString match;
    /*    if (item->GetFocusedLayout())
     match = item->GetFocusedLayout()->GetAllText();
     else if (item->GetLayout())
     match = item->GetLayout()->GetAllText();
     else*/
    match = item->GetLabel(); // Filter label only for now

    if (numericMatch)
      StringUtils::WordToDigits(match);

    size_t pos = StringUtils::FindWords(match.c_str(), trimmedFilter.c_str());
    if (pos != CStdString::npos)
      filteredItems.Add(item);
  }

  items.ClearItems();
  items.Append(filteredItems);

  return items.GetObjectCount() > 0;
}

bool CBackgroundFilterProcessor::GetAdvanceFilteredItems(CFileItemList &items)
{
  // don't run the advanced filter if the filter is empty
  // and there hasn't been a filter applied before which
  // would have to be removed
  CURL url(m_strFilterPath);
  if (m_advancedFilter.IsEmpty() && !url.HasOption("filter"))
    return false;

  CFileItemList resultItems;
  XFILE::CSmartPlaylistDirectory::GetDirectory(m_advancedFilter, resultItems, m_strFilterPath, true);

  // put together a lookup map for faster path comparison
  map<CStdString, CFileItemPtr> lookup;
  for (int j = 0; j < resultItems.Size(); j++)
  {
    CheckThreadInterrupted();
    CStdString itemPath = RemoveParameterFromPath(resultItems[j]->GetPath(), "filter");
    StringUtils::ToLower(itemPath);

    lookup[itemPath] = resultItems[j];
  }

  // loop through all the original items and find
  // those which are still part of the filter
  CFileItemList filteredItems;
  for (int i = 0; i < items.Size(); i++)
  {
    CheckThreadInterrupted();
    CFileItemPtr item = items.Get(i);
    if (item->IsParentFolder())
    {
      filteredItems.Add(item);
      continue;
    }

    // check if the item is part of the resultItems list
    // by comparing their paths (but ignoring any special
    // options because they differ from filter to filter)
    CStdString path = RemoveParameterFromPath(item->GetPath(), "filter");
    StringUtils::ToLower(path);

    map<CStdString, CFileItemPtr>::iterator itItem = lookup.find(path);
    if (itItem != lookup.end())
    {
      // add the item to the list of filtered items
      filteredItems.Add(item);

      // remove the item from the lists
      resultItems.Remove(itItem->second.get());
      lookup.erase(itItem);
    }
  }

  if (resultItems.Size() > 0)
    CLog::Log(LOGWARNING, "CGUIMediaWindow::GetAdvanceFilteredItems(): %d unknown items", resultItems.Size());

  items.ClearItems();
  items.Append(filteredItems);
  items.SetPath(resultItems.GetPath());
  if (resultItems.HasProperty(PROPERTY_PATH_DB))
    items.SetProperty(PROPERTY_PATH_DB, resultItems.GetProperty(PROPERTY_PATH_DB));
  return true;
}

bool CBackgroundFilterProcessor::ApplyWatchedFilter(CFileItemList &items) {
  bool listchanged = false;
  CVideoDatabaseDirectory dir;
  NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());

  // now filter watched items as necessary
  bool filterWatched=false;
  if (node == NODE_TYPE_EPISODES
  ||  node == NODE_TYPE_SEASONS
  ||  node == NODE_TYPE_SETS
  ||  node == NODE_TYPE_TAGS
  ||  node == NODE_TYPE_TITLE_MOVIES
  ||  node == NODE_TYPE_TITLE_TVSHOWS
  ||  node == NODE_TYPE_TITLE_MUSICVIDEOS
  ||  node == NODE_TYPE_RECENTLY_ADDED_EPISODES
  ||  node == NODE_TYPE_RECENTLY_ADDED_MOVIES
  ||  node == NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS)
    filterWatched = true;
  if (!items.IsVideoDb())
    filterWatched = true;
  if (items.GetContent() == "tvshows" &&
     (items.IsSmartPlayList() || items.IsLibraryFolder()))
    node = NODE_TYPE_TITLE_TVSHOWS; // so that the check below works

  int watchMode = CMediaSettings::Get().GetWatchedMode(items.GetContent());

  for (int i = 0; i < items.Size(); i++)
  {
    CheckThreadInterrupted();
    CFileItemPtr item = items.Get(i);

    if(item->HasVideoInfoTag() && (node == NODE_TYPE_TITLE_TVSHOWS || node == NODE_TYPE_SEASONS))
    {
      if (watchMode == WatchedModeUnwatched)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("unwatchedepisodes").asInteger();
      if (watchMode == WatchedModeWatched)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("watchedepisodes").asInteger();
      if (watchMode == WatchedModeAll)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("totalepisodes").asInteger();
      item->SetProperty("numepisodes", item->GetVideoInfoTag()->m_iEpisode);
      listchanged = true;
    }

    if (filterWatched)
    {
      if((watchMode==WatchedModeWatched   && item->GetVideoInfoTag()->m_playCount== 0)
      || (watchMode==WatchedModeUnwatched && item->GetVideoInfoTag()->m_playCount > 0))
      {
        items.Remove(i);
        i--;
        listchanged = true;
      }
    }
  }

  if(node == NODE_TYPE_TITLE_TVSHOWS || node == NODE_TYPE_SEASONS) {
    items.ClearSortState();
  }

  return listchanged;
}

void CBackgroundFilterProcessor::FormatItemLabels(CFileItemList &items, const LABEL_MASKS &labelMasks)
{
  CLabelFormatter fileFormatter(labelMasks.m_strLabelFile, labelMasks.m_strLabel2File);
  CLabelFormatter folderFormatter(labelMasks.m_strLabelFolder, labelMasks.m_strLabel2Folder);
  for (int i=0; i<items.Size(); ++i)
  {
    CheckThreadInterrupted();
    CFileItemPtr pItem=items[i];

    if (pItem->IsLabelPreformated())
      continue;

    if (pItem->m_bIsFolder)
      folderFormatter.FormatLabels(pItem.get());
    else
      fileFormatter.FormatLabels(pItem.get());
  }

  if (items.GetSortMethod() == SortByLabel)
    items.ClearSortState();
}

void CBackgroundFilterProcessor::FormatAndSort(CFileItemList &items) {
  FormatItemLabels(items, m_viewStateLabelMasks);
  items.Sort(m_viewStateSortMethod.sortBy, m_viewStateSortOrder, m_viewStateSortMethod.sortAttributes);
}

CStdString CBackgroundFilterProcessor::RemoveParameterFromPath(const CStdString &strDirectory, const CStdString &strParameter) {
  CURL url(strDirectory);
  if (url.HasOption(strParameter))
  {
    url.RemoveOption(strParameter);
    return url.Get();
  }

  return strDirectory;
}

void CBackgroundFilterProcessor::GetGroupedItems(CFileItemList &items)
{
  std::string group;
  bool mixed = false;
  if (items.HasProperty(PROPERTY_GROUP_BY))
    group = items.GetProperty(PROPERTY_GROUP_BY).asString();
  if (items.HasProperty(PROPERTY_GROUP_MIXED))
    mixed = items.GetProperty(PROPERTY_GROUP_MIXED).asBoolean();

  // group == "none" completely supresses any grouping
  if (!StringUtils::EqualsNoCase(group, "none"))
  {
    CQueryParams params;
    CVideoDatabaseDirectory dir;
    dir.GetQueryParams(items.GetPath(), params);
    VIDEODATABASEDIRECTORY::NODE_TYPE nodeType = CVideoDatabaseDirectory::GetDirectoryChildType(m_strFilterPath);
    if (items.GetContent().Equals("movies") && params.GetSetId() <= 0 &&
        nodeType == NODE_TYPE_TITLE_MOVIES &&
       (CSettings::Get().GetBool("videolibrary.groupmoviesets") || (StringUtils::EqualsNoCase(group, "sets") && mixed)))
    {
      CFileItemList groupedItems;
      if (GroupUtils::Group(GroupBySet, m_strFilterPath, items, groupedItems, GroupAttributeIgnoreSingleItems))
      {
        items.ClearItems();
        items.Append(groupedItems);
      }
    }
  }
}

void CBackgroundFilterProcessor::Run() {
  try {
    unsigned int timeFull = XbmcThreads::SystemClockMillis();
    unsigned int timePart = XbmcThreads::SystemClockMillis();
    CLog::Log(LOGDEBUG, "%s started", __FUNCTION__);

    CheckThreadInterrupted();
    bool filtered = GetFilteredItems(m_strFilter, m_itemList);

    CLog::Log(LOGDEBUG, "%s: ### %s took %d ms", __FUNCTION__, "part 1", XbmcThreads::SystemClockMillis() - timePart);
    timePart = XbmcThreads::SystemClockMillis();

    CheckThreadInterrupted();
    filtered |= ApplyWatchedFilter(m_itemList);

    CLog::Log(LOGDEBUG, "%s: ### %s took %d ms", __FUNCTION__, "part 2", XbmcThreads::SystemClockMillis() - timePart);
    timePart = XbmcThreads::SystemClockMillis();

    CheckThreadInterrupted();
    if(filtered && m_canFilterAdvanced) {
      if(m_itemList.HasProperty(PROPERTY_PATH_DB))
        m_strFilterPath = m_itemList.GetProperty(PROPERTY_PATH_DB).asString();
      else if (m_strFilterPath.empty())
        m_strFilterPath = m_itemList.GetPath();
    }

    CheckThreadInterrupted();
    GetGroupedItems(m_itemList);

    CLog::Log(LOGDEBUG, "%s: ### %s took %d ms", __FUNCTION__, "part 3", XbmcThreads::SystemClockMillis() - timePart);
    timePart = XbmcThreads::SystemClockMillis();

    CheckThreadInterrupted();
    FormatAndSort(m_itemList);

    CLog::Log(LOGDEBUG, "%s: ### %s took %d ms", __FUNCTION__, "part 4", XbmcThreads::SystemClockMillis() - timePart);
    timePart = XbmcThreads::SystemClockMillis();

    CheckThreadInterrupted();
    CStdString filterOption;
    CURL filterUrl(m_strFilterPath);
    if(filterUrl.HasOption("filter"))
      filterOption = filterUrl.GetOption("filter");

    // apply the "filter" option to any folder item so that
    // the filter can be passed down to the sub-directory
    for (int index = 0; index < m_itemList.Size(); index++)
    {
      CheckThreadInterrupted();
      CFileItemPtr pItem = m_itemList.Get(index);

      // if the item is a folder we need to copy the path of
      // the filtered item to be able to keep the applied filters
      if (pItem->m_bIsFolder)
      {
        CURL itemUrl(pItem->GetPath());
        if (!filterOption.empty())
          itemUrl.SetOption("filter", filterOption);
        else
          itemUrl.RemoveOption("filter");

        pItem->SetPath(itemUrl.Get());
      }
    }

    CLog::Log(LOGDEBUG, "%s: ### %s took %d ms", __FUNCTION__, "part 5", XbmcThreads::SystemClockMillis() - timePart);
    timePart = XbmcThreads::SystemClockMillis();

    CheckThreadInterrupted();
    CBackgroundFilterMessageSenderJob* messageSender = new CBackgroundFilterMessageSenderJob(m_itemList, m_selectedDBId);
    AddJob(messageSender);

    CLog::Log(LOGDEBUG, "%s finished in %d ms", __FUNCTION__, XbmcThreads::SystemClockMillis() - timeFull);

  }
  catch (FilterProcessorInterruptException) {
    CLog::Log(LOGERROR, "%s was interrupted", __FUNCTION__);
  }
  catch (...) {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }

  m_bIsFiltering = false;
}

void CBackgroundFilterProcessor::CheckThreadInterrupted() {
  if(m_bStop)
    throw FilterProcessorInterruptException();
}

void CBackgroundFilterProcessor::StartFiltering(
    const CStdString &filter, const CStdString &filterPath, const CFileItemList& items,
    const CGUIViewState* pViewState, const CSmartPlaylist& advancedFilter, const int selectedDBId, bool canFilterAdvanced) {

  StopFiltering();

  if(items.Size() == 0)
    return;

  CSingleLock lock(m_lock);

  m_itemList.Copy(items, false);
  m_itemList.Append(items);

  m_strFilter = filter;
  m_strFilterPath = filterPath;
  m_canFilterAdvanced = canFilterAdvanced;
  m_advancedFilter = advancedFilter;
  m_selectedDBId = selectedDBId;

  if(pViewState) {
    pViewState->GetSortMethodLabelMasks(m_viewStateLabelMasks);
    m_viewStateSortMethod = pViewState->GetSortMethod();
    m_viewStateSortOrder = pViewState->GetDisplaySortOrder();
  }

  m_bStop = false;
  m_bIsFiltering = true;

  m_thread = new CThread(this, "BackgroundFilterProcessor");
  m_thread->Create();
#ifndef TARGET_POSIX
  m_thread->SetPriority(THREAD_PRIORITY_BELOW_NORMAL);
#endif
}

void CBackgroundFilterProcessor::StopFiltering() {
  CLog::Log(LOGERROR, "%s - Stopping thread", __FUNCTION__);

  m_bStop = true;

  if (m_thread) {
    m_thread->StopThread();
    delete m_thread;
    m_thread = NULL;
  }

  m_itemList.Clear();
  m_bIsFiltering = false;

  CLog::Log(LOGERROR, "%s - Stopped thread", __FUNCTION__);
}

bool CBackgroundFilterProcessor::IsFiltering() {
  return m_bIsFiltering;
}

