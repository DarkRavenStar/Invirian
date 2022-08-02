using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Sharpmake;

[module: Sharpmake.Include("Common.sharpmake.cs")]

namespace InvirianEngine
{
	[Sharpmake.Export]
	public class stb : Project
	{
		public stb()
		{
			Name = "stb";
			IsFileNameToLower = false;
			SourceRootPath = @"[project.SharpmakeCsPath]\..\ThirdParty\stb\";

			/*
			SourceFiles.Add
			(
				@"[project.SharpmakeCsPath]\..\ThirdParty\stb\stb_image.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\stb\stb_image_resize.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\stb\stb_image_write.h"
			);
			*/
			
			AddTargets
			(
				new Target
				(
					Platform.win64,
					DevEnv.vs2019,
					Optimization.Debug | Optimization.Release
				)
			);
		}

		[Configure]
		public void ConfigureAll(Configuration conf, Target target)
		{
			conf.SolutionFolder = "Dependency";
			//Teena - we want intelisense but not building
			//conf.IsExcludedFromBuild = true;
			//conf.Output = Configuration.OutputType.Lib;
			//conf.Output = Configuration.OutputType.None;
			conf.Output = Configuration.OutputType.Utility;

			//conf.TargetLibraryPath = @"[conf.ProjectPath]\obj\[project.Name]\[target.Platform]\[target.Optimization]\[target.DevEnv]";
			//conf.IntermediatePath = @"[conf.ProjectPath]\output\[project.Name]\[target.Platform]\[target.Optimization]\[target.DevEnv]";

			conf.ProjectFileName = "stb";
			conf.ProjectPath = @"[project.SharpmakeCsPath]\..\ProjectSetup\";
			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\stb\");
		}
	}
}
