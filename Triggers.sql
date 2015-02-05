CREATE PROCEDURE `xbmc_video90`.`update_seasonview`(IN idSeason int(11))
    MODIFIES SQL DATA
BEGIN
  DECLARE exist         int DEFAULT TRUE;
  DECLARE idShow	int(11);
  DECLARE season	int(11);
  DECLARE strPath	text;
  DECLARE showTitle	text;
  DECLARE plot          text;
  DECLARE premiered	text;
  DECLARE genre         text;
  DECLARE strStudio	text;
  DECLARE mpaa          text;
  DECLARE episodes	bigint(21);
  DECLARE playCount	bigint(21);

  DECLARE seasonviewCursor CURSOR FOR 
    SELECT seasons.idShow, seasons.season, 
           tvshowview.strPath, tvshowview.c00, tvshowview.c01, tvshowview.c05, tvshowview.c08, tvshowview.c14, tvshowview.c13, 
           COUNT(DISTINCT episodeview.idEpisode), COUNT(files.playCount)
    FROM seasons 
      JOIN tvshowview ON tvshowview.idShow = seasons.idShow
      JOIN episodeview ON episodeview.idShow = seasons.idShow AND episodeview.c12 = seasons.season
      JOIN files ON files.idFile = episodeview.idFile
    WHERE seasons.idSeason = idSeason
    GROUP BY seasons.idSeason;

  DECLARE CONTINUE HANDLER FOR NOT FOUND SET exist = FALSE;

  OPEN seasonviewCursor;
  FETCH seasonviewCursor INTO idShow, season, strPath, showTitle, plot, premiered, genre, strStudio, mpaa, episodes, playCount;

  IF exist THEN
    INSERT INTO seasonview(idSeason, idShow, season, strPath, showTitle, plot, premiered, genre, strStudio, mpaa, episodes, playCount)
      VALUES(idSeason, idShow, season, strPath, showTitle, plot, premiered, genre, strStudio, mpaa, episodes, playCount)
      ON DUPLICATE KEY UPDATE 
         idShow = VALUES(idShow), 
         season = VALUES(season), 
         strPath = VALUES(strPath), 
         showTitle = VALUES(showTitle), 
         plot = VALUES(plot), 
         premiered = VALUES(premiered), 
         genre = VALUES(genre), 
         strStudio = VALUES(strStudio), 
         mpaa = VALUES(mpaa), 
         episodes = VALUES(episodes), 
         playCount = VALUES(playCount);
  ELSE
    DELETE FROM seasonview WHERE seasonview.idSeason = idSeason;
  END IF;

  CLOSE seasonviewCursor;
END



CREATE PROCEDURE `xbmc_video90`.`update_seasonview_fileid`(IN idFile int(11))
    MODIFIES SQL DATA
BEGIN
  DECLARE done int DEFAULT FALSE;
  DECLARE idSeason int(11);
  DECLARE seasonsCursor CURSOR FOR 
    SELECT seasons.idSeason 
      FROM seasons, episode
      WHERE seasons.idShow = episode.idShow
        AND seasons.season = episode.c12
        AND episode.idFile = idFile;

  DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

  OPEN seasonsCursor;

  cursorLoop: LOOP
    FETCH seasonsCursor INTO idSeason;

    IF done THEN
      LEAVE cursorLoop;
    END IF;
    
    CALL update_seasonview(idSeason);
  END LOOP;

  CLOSE seasonsCursor;
END



CREATE PROCEDURE `xbmc_video90`.`update_seasonview_pathid`(IN idPath int(11))
    MODIFIES SQL DATA
BEGIN
  DECLARE done int DEFAULT FALSE;
  DECLARE idSeason int(11);
  DECLARE seasonsCursor CURSOR FOR 
    SELECT seasons.idSeason 
      FROM seasons, tvshowlinkpath 
      WHERE seasons.idShow = tvshowlinkpath.idShow
        AND tvshowlinkpath.idPath = idPath
    UNION
    SELECT seasons.idSeason 
      FROM seasons, episode, files
      WHERE seasons.idShow = episode.idShow
        AND seasons.season = episode.c12
        AND episode.idFile = files.idFile
        AND files.idPath = idPath;

  DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

  OPEN seasonsCursor;

  cursorLoop: LOOP
    FETCH seasonsCursor INTO idSeason;

    IF done THEN
      LEAVE cursorLoop;
    END IF;
    
    CALL update_seasonview(idSeason);
  END LOOP;

  CLOSE seasonsCursor;
END



CREATE PROCEDURE `xbmc_video90`.`update_seasonview_tvshowid`(IN idShow int(11))
    MODIFIES SQL DATA
BEGIN
  DECLARE done int DEFAULT FALSE;
  DECLARE idSeason int(11);
  DECLARE seasonsCursor CURSOR FOR 
    SELECT seasons.idSeason 
      FROM seasons 
      WHERE seasons.idShow = idShow;

  DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

  OPEN seasonsCursor;

  cursorLoop: LOOP
    FETCH seasonsCursor INTO idSeason;

    IF done THEN
      LEAVE cursorLoop;
    END IF;
    
    CALL update_seasonview(idSeason);
  END LOOP;

  CLOSE seasonsCursor;
END



CREATE PROCEDURE `xbmc_video90`.`update_seasonview_tvshowid_season`(IN idShow int(11), IN season int(11))
    MODIFIES SQL DATA
BEGIN
  DECLARE done int DEFAULT FALSE;
  DECLARE idSeason int(11);
  DECLARE seasonsCursor CURSOR FOR 
    SELECT seasons.idSeason 
      FROM seasons
      WHERE seasons.idShow = idShow
        AND seasons.season = season;

  DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

  OPEN seasonsCursor;

  cursorLoop: LOOP
    FETCH seasonsCursor INTO idSeason;

    IF done THEN
      LEAVE cursorLoop;
    END IF;
    
    CALL update_seasonview(idSeason);
  END LOOP;

  CLOSE seasonsCursor;
END



CREATE PROCEDURE `xbmc_video90`.`update_tvshowview`(IN idShow int(11))
    MODIFIES SQL DATA
BEGIN
  DECLARE exist         int DEFAULT TRUE;
  DECLARE c00           text;
  DECLARE c01	        text;
  DECLARE c02	        text;
  DECLARE c03	        text;
  DECLARE c04	        text;
  DECLARE c05	        text;
  DECLARE c06	        text;
  DECLARE c07	        text;
  DECLARE c08	        text;
  DECLARE c09	        text;
  DECLARE c10	        text;
  DECLARE c11	        text;
  DECLARE c12	        text;
  DECLARE c13	        text;
  DECLARE c14	        text;
  DECLARE c15	        text;
  DECLARE c16	        text;
  DECLARE c17	        text;
  DECLARE c18	        text;
  DECLARE c19	        text;
  DECLARE c20	        text;
  DECLARE c21	        text;
  DECLARE c22	        text;
  DECLARE c23	        text;
  DECLARE idParentPath	int(11);
  DECLARE strPath	text;
  DECLARE dateAdded	text;
  DECLARE lastPlayed	text;
  DECLARE totalCount	bigint(21);
  DECLARE watchedcount	bigint(21);
  DECLARE totalSeasons	bigint(21);

  DECLARE tvshowviewCursor CURSOR FOR 
    SELECT tvshow.c00, tvshow.c01, tvshow.c02, tvshow.c03, tvshow.c04, tvshow.c05, tvshow.c06, tvshow.c07, tvshow.c08, tvshow.c09, tvshow.c10, tvshow.c11, 
           tvshow.c12, tvshow.c13, tvshow.c14, tvshow.c15, tvshow.c16, tvshow.c17, tvshow.c18, tvshow.c19, tvshow.c20, tvshow.c21, tvshow.c22, tvshow.c23, 
           path.idParentPath, path.strPath,
           tvshowcounts.dateAdded, tvshowcounts.lastPlayed, tvshowcounts.totalCount, tvshowcounts.watchedcount, tvshowcounts.totalSeasons
    FROM tvshow
      LEFT JOIN tvshowlinkpath ON tvshowlinkpath.idShow = tvshow.idShow
      LEFT JOIN path ON path.idPath = tvshowlinkpath.idPath
           JOIN tvshowcounts ON tvshow.idShow = tvshowcounts.idShow
    WHERE tvshow.idShow = idShow
    GROUP BY tvshow.idShow;

  DECLARE CONTINUE HANDLER FOR NOT FOUND SET exist = FALSE;

  OPEN tvshowviewCursor;
  FETCH tvshowviewCursor INTO c00, c01, c02, c03, c04, c05, c06, c07, c08, c09, c10, c11, c12, c13, c14, c15, c16, c17, c18, c19, c20, c21, c22, c23, 
                              idParentPath, strPath, dateAdded, lastPlayed, totalCount, watchedcount, totalSeasons;
  IF exist THEN
    INSERT INTO tvshowview(idShow, c00, c01, c02, c03, c04, c05, c06, c07, c08, c09, c10, c11, c12, c13, c14, c15, c16, c17, c18, c19, c20, c21, c22, c23, 
                           idParentPath, strPath, dateAdded, lastPlayed, totalCount, watchedcount, totalSeasons)
      VALUES(idShow, c00, c01, c02, c03, c04, c05, c06, c07, c08, c09, c10, c11, c12, c13, c14, c15, c16, c17, c18, c19, c20, c21, c22, c23, 
             idParentPath, strPath, dateAdded, lastPlayed, totalCount, watchedcount, totalSeasons)
      ON DUPLICATE KEY UPDATE 
         c00 = VALUES(c00),
         c01 = VALUES(c01),
         c02 = VALUES(c02),
         c03 = VALUES(c03),
         c04 = VALUES(c04),
         c05 = VALUES(c05),
         c06 = VALUES(c06),
         c07 = VALUES(c07),
         c08 = VALUES(c08),
         c09 = VALUES(c09),
         c10 = VALUES(c10),
         c11 = VALUES(c11),
         c12 = VALUES(c12),
         c13 = VALUES(c13),
         c14 = VALUES(c14),
         c15 = VALUES(c15),
         c16 = VALUES(c16),
         c17 = VALUES(c17),
         c18 = VALUES(c18),
         c19 = VALUES(c19),
         c20 = VALUES(c20),
         c21 = VALUES(c21),
         c22 = VALUES(c22),
         c23 = VALUES(c23),
         idParentPath = VALUES(idParentPath),
         strPath = VALUES(strPath),
         dateAdded = VALUES(dateAdded),
         lastPlayed = VALUES(lastPlayed),
         totalCount = VALUES(totalCount),
         watchedcount = VALUES(watchedcount),
         totalSeasons = VALUES(totalSeasons);
  ELSE
    DELETE FROM tvshowview WHERE tvshowview.idShow = idShow;
  END IF;

  CLOSE tvshowviewCursor;
END



CREATE PROCEDURE `xbmc_video90`.`update_tvshowview_fileid`(IN idFile int(11))
    MODIFIES SQL DATA
BEGIN
  DECLARE done int DEFAULT FALSE;
  DECLARE idShow int(11);
  DECLARE tvshowCursor CURSOR FOR 
    SELECT episode.idShow 
      FROM episode
      WHERE episode.idFile = idFile;

  DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

  OPEN tvshowCursor;

  cursorLoop: LOOP
    FETCH tvshowCursor INTO idShow;

    IF done THEN
      LEAVE cursorLoop;
    END IF;
    
    CALL update_tvshowview(idShow);
  END LOOP;

  CLOSE tvshowCursor;
END



CREATE PROCEDURE `xbmc_video90`.`update_tvshowview_pathid`(IN idPath int(11))
    MODIFIES SQL DATA
BEGIN
  DECLARE done int DEFAULT FALSE;
  DECLARE idShow int(11);
  DECLARE tvshowCursor CURSOR FOR 
    SELECT tvshowlinkpath.idShow
      FROM tvshowlinkpath 
      WHERE tvshowlinkpath.idPath = idPath
    UNION
    SELECT episode.idShow 
      FROM episode, files
      WHERE episode.idFile = files.idFile
        AND files.idPath = idPath;

  DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

  OPEN tvshowCursor;

  cursorLoop: LOOP
    FETCH tvshowCursor INTO idShow;

    IF done THEN
      LEAVE cursorLoop;
    END IF;
    
    CALL update_tvshowview(idShow);
  END LOOP;

  CLOSE tvshowCursor;
END



-- table: episode --

CREATE TRIGGER `xbmc_video90`.`after_insert_episode`
AFTER INSERT ON 
xbmc_video90.episode
FOR EACH ROW BEGIN
  CALL update_tvshowview(NEW.idShow);
  CALL update_seasonview_tvshowid_season(NEW.idShow, NEW.c12);
END

CREATE TRIGGER `xbmc_video90`.`after_update_episode`
AFTER UPDATE ON 
xbmc_video90.episode
FOR EACH ROW BEGIN
  CALL update_tvshowview(NEW.idShow);
  CALL update_seasonview_tvshowid_season(NEW.idShow, NEW.c12);
END

CREATE TRIGGER `xbmc_video90`.`delete_episode`
AFTER DELETE ON 
xbmc_video90.episode
FOR EACH ROW BEGIN 
  DELETE FROM actorlinkepisode WHERE idEpisode=old.idEpisode; 
  DELETE FROM directorlinkepisode WHERE idEpisode=old.idEpisode; 
  DELETE FROM writerlinkepisode WHERE idEpisode=old.idEpisode; 
  DELETE FROM art WHERE media_id=old.idEpisode AND media_type='episode'; 

  CALL update_tvshowview(OLD.idShow);
  CALL update_seasonview_tvshowid_season(OLD.idShow, OLD.c12);
END



-- table: files --

CREATE TRIGGER `xbmc_video90`.`after_insert_files`
AFTER INSERT ON 
xbmc_video90.files
FOR EACH ROW BEGIN
  CALL update_tvshowview_fileid(NEW.idFile);
  CALL update_seasonview_fileid(NEW.idFile);
END

CREATE TRIGGER `xbmc_video90`.`after_update_files`
AFTER UPDATE ON 
xbmc_video90.files
FOR EACH ROW BEGIN
  CALL update_tvshowview_fileid(NEW.idFile);
  CALL update_seasonview_fileid(NEW.idFile);
END

CREATE TRIGGER `xbmc_video90`.`after_delete_files`
AFTER DELETE ON 
xbmc_video90.files
FOR EACH ROW BEGIN
  CALL update_tvshowview_fileid(OLD.idFile);
  CALL update_seasonview_fileid(OLD.idFile);
END



-- table: path --

CREATE TRIGGER `xbmc_video90`.`after_insert_path`
AFTER INSERT ON 
xbmc_video90.path
FOR EACH ROW BEGIN
  CALL update_tvshowview_pathid(NEW.idPath);
  CALL update_seasonview_pathid(NEW.idPath);
END

CREATE TRIGGER `xbmc_video90`.`after_update_path`
AFTER UPDATE ON 
xbmc_video90.path
FOR EACH ROW BEGIN
  CALL update_tvshowview_pathid(NEW.idPath);
  CALL update_seasonview_pathid(NEW.idPath);
END

CREATE TRIGGER `xbmc_video90`.`after_delete_path`
AFTER DELETE ON 
xbmc_video90.path
FOR EACH ROW BEGIN
  CALL update_tvshowview_pathid(OLD.idPath);
  CALL update_seasonview_pathid(OLD.idPath);
END



-- table: seasons --

CREATE TRIGGER `xbmc_video90`.`after_insert_seasons`
AFTER INSERT ON 
xbmc_video90.seasons
FOR EACH ROW BEGIN
  CALL update_seasonview(NEW.idSeason);
END

CREATE TRIGGER `xbmc_video90`.`after_update_seasons`
AFTER UPDATE ON 
xbmc_video90.seasons
FOR EACH ROW BEGIN
  CALL update_seasonview(NEW.idSeason);
END

CREATE TRIGGER `xbmc_video90`.`delete_season`
AFTER DELETE ON 
xbmc_video90.seasons
FOR EACH ROW BEGIN 
  DELETE FROM art WHERE media_id = OLD.idSeason AND media_type = 'season';
  CALL update_seasonview(OLD.idSeason);
END



-- table: tvshow --

CREATE TRIGGER `xbmc_video90`.`after_insert_tvshow`
AFTER INSERT ON 
xbmc_video90.tvshow
FOR EACH ROW BEGIN
  CALL update_tvshowview(NEW.idShow);
  CALL update_seasonview_tvshowid(NEW.idShow);
END

CREATE TRIGGER `xbmc_video90`.`after_update_tvshow`
AFTER UPDATE ON 
xbmc_video90.tvshow
FOR EACH ROW BEGIN
  CALL update_tvshowview(NEW.idShow);
  CALL update_seasonview_tvshowid(NEW.idShow);
END

CREATE TRIGGER `xbmc_video90`.`delete_tvshow`
AFTER DELETE ON 
xbmc_video90.tvshow
FOR EACH ROW BEGIN 
  DELETE FROM actorlinktvshow WHERE idShow=old.idShow; 
  DELETE FROM directorlinktvshow WHERE idShow=old.idShow; 
  DELETE FROM tvshowlinkpath WHERE idShow=old.idShow; 
  DELETE FROM genrelinktvshow WHERE idShow=old.idShow; 
  DELETE FROM movielinktvshow WHERE idShow=old.idShow; 
  DELETE FROM seasons WHERE idShow=old.idShow; 
  DELETE FROM art WHERE media_id=old.idShow AND media_type='tvshow'; 
  DELETE FROM taglinks WHERE idMedia=old.idShow AND media_type='tvshow'; 

  CALL update_tvshowview(OLD.idShow);
  CALL update_seasonview_tvshowid(OLD.idShow);
END



-- table: tvshowlinkpath --

CREATE TRIGGER `xbmc_video90`.`after_insert_tvshowlinkpath`
AFTER INSERT ON 
xbmc_video90.tvshowlinkpath
FOR EACH ROW BEGIN
  CALL update_tvshowview(NEW.idShow);
  CALL update_seasonview_tvshowid(NEW.idShow);
END

CREATE TRIGGER `xbmc_video90`.`after_update_tvshowlinkpath`
AFTER UPDATE ON 
xbmc_video90.tvshowlinkpath
FOR EACH ROW BEGIN
  IF NEW.idShow != OLD.idShow THEN
    CALL update_tvshowview(OLD.idShow);
    CALL update_seasonview_tvshowid(OLD.idShow);
  END IF;

  CALL update_tvshowview(NEW.idShow);
  CALL update_seasonview_tvshowid(NEW.idShow);
END

CREATE TRIGGER `xbmc_video90`.`after_delete_tvshowlinkpath`
AFTER DELETE ON 
xbmc_video90.tvshowlinkpath
FOR EACH ROW BEGIN
  CALL update_tvshowview(OLD.idShow);
  CALL update_seasonview_tvshowid(OLD.idShow);
END
