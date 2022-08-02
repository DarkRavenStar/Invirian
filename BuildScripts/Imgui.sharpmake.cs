using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Sharpmake;

[module: Sharpmake.Include("Common.sharpmake.cs")]
[module: Sharpmake.Include("GLFW.sharpmake.cs")]
[module: Sharpmake.Include("VulkanSDK.sharpmake.cs")]

namespace InvirianEngine
{
	[Sharpmake.Generate]
	public class Imgui : Project
	{
		public Imgui()
		{
			Name = "ImGui";
			IsFileNameToLower = false;
			//SourceRootPath = @"[project.SharpmakeCsPath]\..\ThirdParty\imgui\";

			SourceFiles.Add
			(
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imconfig.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imgui.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imgui.cpp",
				
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imgui_draw.cpp",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imgui_internal.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imgui_tables.cpp",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imgui_widgets.cpp",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imstb_rectpack.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imstb_textedit.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imstb_truetype.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\imgui_demo.cpp",
				
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\backends\imgui_impl_vulkan.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\backends\imgui_impl_vulkan.cpp",

				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\backends\imgui_impl_glfw.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\backends\imgui_impl_glfw.cpp"
			);
			
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
			//conf.AddPrivateDependency<GLFWLib>(target);
			conf.AddPrivateDependency<GLFW>(target);
			//conf.AddPrivateDependency<GLFWLib>(target);
			conf.AddPrivateDependency<VulkanSDK>(target);

			conf.SolutionFolder = "Dependency";
			//Teena - we want intelisense but not building
			//conf.IsExcludedFromBuild = true;
			conf.Output = Configuration.OutputType.Lib;
			//conf.Output = Configuration.OutputType.None;
			//conf.Output = Configuration.OutputType.Utility;

			//conf.TargetLibraryPath = @"[conf.ProjectPath]\obj\[project.Name]\[target.Platform]\[target.Optimization]\[target.DevEnv]";
			//conf.IntermediatePath = @"[conf.ProjectPath]\output\[project.Name]\[target.Platform]\[target.Optimization]\[target.DevEnv]";

			conf.ProjectFileName = "ImGui";
			conf.ProjectPath = @"[project.SharpmakeCsPath]\..\ProjectSetup\";
			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\");
			conf.LibraryFiles.Add("ImGui");
		}
	}
}
