﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Wii;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformRawkFile : Engine
	{
		public static PlatformRawkFile Instance;
		public static void Initialise()
		{
			Instance = new PlatformRawkFile();

			PlatformDetection.AddSelection(Instance);
			PlatformDetection.DetectDirectoryNode += new Action<string, DirectoryNode, List<Pair<Engine, Game>>>(PlatformDetection_DetectDirectoryNode);
		}

		static void PlatformDetection_DetectDirectoryNode(string path, DirectoryNode root, List<Pair<Engine, Game>> platforms)
		{
			if ((root.Find("data", SearchOption.AllDirectories, true) != null && root.Find("chart", SearchOption.AllDirectories, true) != null) || root.Find("songdata", SearchOption.AllDirectories, true) != null)
				platforms.Add(new Pair<Engine, Game>(Instance, Game.Unknown));
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "RawkSD Song Archive"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public bool AddSongNew(PlatformData data, SongData song, ProgressIndicator progress)
		{
			FormatData formatdata = new TemporaryFormatData(song, data);

			DirectoryNode dir = data.Session["songdir"] as DirectoryNode;
			foreach (FileNode file in dir.Files)
				formatdata.SetStream(file.Name, file.Data);

			data.AddSong(formatdata);

			return true;
		}

		public bool AddSongOld(PlatformData data, SongData song, ProgressIndicator progress)
		{
			FormatData formatdata = new TemporaryFormatData(song, data);
			
			formatdata.Song.Data.SetValue("RawkSD2Compatibility", true);
			formatdata.Song.Version = 0;

			DirectoryNode dir = data.Session["songdir"] as DirectoryNode;

			FileNode audiofile = dir.Navigate("audio") as FileNode;
			FileNode albumfile = dir.Navigate("album") as FileNode;
			FileNode weightsfile = dir.Navigate("weights") as FileNode;
			FileNode panfile = dir.Navigate("pan") as FileNode;
			FileNode milofile = dir.Navigate("milo") as FileNode;
			FileNode chartfile = dir.Navigate("chart") as FileNode;
			FileNode previewfile = dir.Navigate("preview") as FileNode;

			if (audiofile != null) {
				AudioFormat audio = HarmonixMetadata.GetAudioFormat(song);
				AudioFormatRB2Mogg.Instance.Create(formatdata, audiofile.Data, previewfile.Data, null);
			}

			if (albumfile != null)
				song.AlbumArt = WiiImage.Create(new EndianReader(albumfile.Data, Endianness.LittleEndian)).Bitmap;

			if (chartfile != null)
				ChartFormatRB.Instance.Create(formatdata, chartfile == null ? null : chartfile.Data, panfile == null ? null : panfile.Data, weightsfile == null ? null : weightsfile.Data, milofile == null ? null : milofile.Data, false, false, Game.RockBand2);

			data.AddSong(formatdata);

			return true;
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);

			DirectoryNode dir = data.GetDirectoryStructure(path);

			List<DirectoryNode> dirs = new List<DirectoryNode>();
			dirs.Add(dir);
			dirs.AddRange(dir.Directories);

			progress.NewTask(dirs.Count);

			foreach (DirectoryNode songdir in dirs) {
				data.Session["songdir"] = songdir;

				FileNode datafile = songdir.Navigate("data", false, true) as FileNode;
				FileNode newdatafile = songdir.Navigate("songdata", false, true) as FileNode;
				if (datafile == null && newdatafile == null) {
					Exceptions.Warning("Unable to find a custom in " + songdir.Filename);
					continue;
				}

				try {
					SongData song = null;
					if (datafile != null) {
						song = HarmonixMetadata.GetSongData(data, DTB.Create(new EndianReader(datafile.Data, Endianness.LittleEndian)));
						datafile.Data.Close();
						AddSongOld(data, song, progress);
					} else {
						song = SongData.Create(newdatafile.Data);
						newdatafile.Data.Close();
						AddSongNew(data, song, progress);
					}
				} catch (Exception exception) {
					Exceptions.Error(exception, "Unable to open the custom from " + songdir.Filename);
				}

				progress.Progress();
			}
			progress.EndTask();

			return data;
		}

		public override FormatData CreateSong(PlatformData data, SongData song)
		{
			return new TemporaryFormatData(song, data);
		}

		public override void SaveSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			base.SaveSong(data, formatdata, progress);
		}

		public bool IsRawkSD2(SongData song)
		{
			return song.Data.GetValue<bool>("RawkSD2Compatibility") || song.Version == 0;
		}
	}
}