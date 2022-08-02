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
	public class BuildScriptProject : CSharpProject
	{
		public BuildScriptProject()
		{
			Name = "BuildScript";

			AddTargets
			(
				new Target
				(
					Platform.win64,
					DevEnv.vs2019,
					Optimization.Debug | Optimization.Release
				)
			);

			SourceRootPath = @"[project.SharpmakeCsPath]\";
			AdditionalSourceRootPaths.Add("../ThirdParty/Sharpmake/Sharpmake");
		}

		[Configure]
		public void ConfigureAll(Configuration conf, Target target)
		{
			//Teena - we want intelisense but not building
			conf.IsExcludedFromBuild = true;
			conf.Output = Configuration.OutputType.None;
			conf.ProjectFileName = "BuildScript";
			conf.ProjectPath = @"[project.SharpmakeCsPath]\..\ProjectSetup\";
			//Console.Write("Vulkan sdk path: " + VulkanSDKPath);

			//conf.IncludePaths.Add(VulkanSDKPath + "/Include");
			//conf.LibraryPaths.Add(VulkanSDKPath + "/Lib");
			//conf.LibraryFiles.Add(@"vulkan-1.lib");
			//conf.ReferencesByPath.Add("D:\\Game Development\\Invirian Engine\\Invirian\\ProjectSetup\\Test\\Binary\\Sharpmake.dll");
			//conf.ReferencesByPath.Add("D:\\Game Development\\Invirian Engine\\Invirian\\ThirdParty\\Sharpmake\\Sharpmake\\Sharpmake.csproj");

		}
	}

	[Sharpmake.Generate]
	public class RunBuildScriptProject : CSharpProject
	{
		public RunBuildScriptProject()
		{
			Name = "RunBuildScript";

			AddTargets
			(
				new Target
				(
					Platform.win64,
					DevEnv.vs2019,
					Optimization.Debug | Optimization.Release,
					buildSystem: BuildSystem.MSBuild | BuildSystem.FastBuild
				)
			);

			SourceRootPath = @"[project.SharpmakeCsPath]\";
			//AdditionalSourceRootPaths.Add("../ThirdParty/Sharpmake/Sharpmake");
		}

		[Configure]
		public void ConfigureAll(Configuration conf, Target target)
		{
			//Teena - we want intelisense but not building
			conf.IsExcludedFromBuild = true;
			conf.Output = Configuration.OutputType.None;
			conf.ProjectFileName = "RunBuildScript";
			conf.ProjectPath = @"[project.SharpmakeCsPath]\..\ProjectSetup\";
			//Console.Write("Vulkan sdk path: " + VulkanSDKPath);

			//conf.IncludePaths.Add(VulkanSDKPath + "/Include");
			//conf.LibraryPaths.Add(VulkanSDKPath + "/Lib");
			//conf.LibraryFiles.Add(@"vulkan-1.lib");
			//conf.ReferencesByPath.Add("D:\\Game Development\\Invirian Engine\\Invirian\\ProjectSetup\\Test\\Binary\\Sharpmake.dll");
			//conf.ReferencesByPath.Add("D:\\Game Development\\Invirian Engine\\Invirian\\ThirdParty\\Sharpmake\\Sharpmake\\Sharpmake.csproj");

		}
	}
}
