﻿using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.IO.Compression;
using ConsoleHaxx.Common;
using ICSharpCode.SharpZipLib.Core;
using ICSharpCode.SharpZipLib.Zip.Compression;
using ICSharpCode.SharpZipLib.Zip.Compression.Streams;

namespace ConsoleHaxx.Harmonix
{
	public class Milo
	{
		private void InflateStream(Stream i, Stream o)
		{
			InflaterInputStream inf = new InflaterInputStream(i, new Inflater(true), 0x400);
			Util.StreamCopy(o, inf, long.MaxValue);
			inf.Flush();
		}

		private int DeflateStream(Stream i, Stream o)
		{
			DeflaterOutputStream def = new DeflaterOutputStream(o, new Deflater(9, true), 0x400);
			int ret = (int)Util.StreamCopy(def, i);
			def.IsStreamOwner = false;
			def.Close();
			return ret;
		}

		public const uint MagicCompressed = 0xAFDEBECB;
		public const uint MagicUncompressed = 0xAFDEBECA;

		public bool Compressed;
		public int DataOffset;

		public List<Stream> Data;

		public Milo()
		{
			Data = new List<Stream>();
		}

		public Milo(EndianReader reader) : this()
		{
			uint magic = new EndianReader(reader.Base, Endianness.BigEndian).ReadUInt32();
			if (magic == MagicCompressed)
				Compressed = true;
			else if (magic == MagicUncompressed)
				Compressed = false;
			else
				throw new NotSupportedException();

			DataOffset = reader.ReadInt32();
			int parts = reader.ReadInt32();
			int uncompressedsize = reader.ReadInt32(); // Uncompressed data size

			int offset = DataOffset;

			for (int i = 0; i < parts; i++) {
				reader.Position = 0x10 + i * 0x04;
				int size = reader.ReadInt32();
				Stream data = new Substream(reader.Base, offset, size);
				offset += size;
				if (Compressed) {
					Stream old = data;
					data = new TemporaryStream();
					InflateStream(old, data);
					data.Position = 0;
				}
				Data.Add(data);
			}
		}

		public void Save(EndianReader writer)
		{
			new EndianReader(writer.Base, Endianness.BigEndian).Write(Compressed ? MagicCompressed : MagicUncompressed);

			writer.Write((int)DataOffset);
			writer.Write((int)Data.Count);

			long oldposition = writer.Position;

			writer.PadTo(DataOffset);

			List<int> sizetable = new List<int>();

			int totalsize = 0;

			foreach (Stream str in Data) {
				str.Position = 0;
				long beforepos = writer.Position;
				int size = 0;
				if (Compressed) {
					size = DeflateStream(str, writer.Base);
				} else {
					size = (int)Util.StreamCopy(writer.Base, str);
				}
				sizetable.Add((int)(writer.Position - beforepos));
				totalsize += size;
			}

			writer.Position = oldposition;
			writer.Write(totalsize);
			foreach (int size in sizetable)
				writer.Write(size);
		}
	}
}
