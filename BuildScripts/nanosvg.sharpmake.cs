using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Sharpmake;

[module: Sharpmake.Include("Common.sharpmake.cs")]

namespace InvirianEngine
{
	[Sharpmake.Generate]
	public class nanosvg : Project
	{
		public nanosvg()
		{
			Name = "nanosvg";
			IsFileNameToLower = false;
			SourceRootPath = @"[project.SharpmakeCsPath]\..\ThirdParty\nanosvg\src\";
			
			
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

			conf.ProjectFileName = "nanosvg";
			conf.ProjectPath = @"[project.SharpmakeCsPath]\..\ProjectSetup\";
			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\nanosvg\src\");
		}
	}
}
