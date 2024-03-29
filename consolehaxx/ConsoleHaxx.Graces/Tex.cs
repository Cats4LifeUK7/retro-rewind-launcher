﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Graces
{
	public class Tex
	{
		public static Txm Create(Stream stream)
		{
			FPS4 fps = new FPS4(stream);
			if (fps.Type == "txmv") {
				if (fps.Root.ChildCount != 2) {
					FileNode file = fps.Root.Files.FirstOrDefault(f => f.Name.ToUpper().EndsWith(".TTX"));
					if (file != null)
						fps = new FPS4(file.Data);
					else
						throw new FormatException();
				}

				return new Txm((fps.Root[0] as FileNode).Data, (fps.Root[1] as FileNode).Data);
			} else if (fps.Type == "pktx") {
				return Create((fps.Root[0] as FileNode).Data);
			}

			throw new FormatException();
		}
	}
}
