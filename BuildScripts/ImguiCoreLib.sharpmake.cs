using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Sharpmake;

[module: Sharpmake.Include("Imgui.sharpmake.cs")]
[module: Sharpmake.Include("GLFW.sharpmake.cs")]

namespace InvirianEngine
{
    [Sharpmake.Generate]
    public class ImguiCoreLib : Project
    {
        public ImguiCoreLib()
        {
            Name = "ImguiCoreLib";
			IsFileNameToLower = false;
			SourceRootPath = @"[project.SharpmakeCsPath]\..\Plugins\ImguiLibCore";

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
			conf.SolutionFolder = "Plugins";
			conf.Output = Configuration.OutputType.Lib;
            conf.ProjectFileName = "ImguiCoreLib";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\..\ProjectSetup\";

			conf.AddPublicDependency<Imgui>(target);
			conf.AddPublicDependency<GLFWLib>(target);
			conf.AddPrivateDependency<VulkanSDK>(target);

			//conf.TargetLibraryPath = @"[conf.ProjectPath]\obj\[project.Name]\[target.Platform]\[target.Optimization]\[target.DevEnv]";
			//conf.IntermediatePath = @"[conf.ProjectPath]\output\[project.Name]\[target.Platform]\[target.Optimization]\[target.DevEnv]";
			//conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\Plugins\ImguiLibCore");
		}
    }
}
