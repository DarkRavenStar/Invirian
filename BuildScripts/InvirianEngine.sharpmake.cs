using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Sharpmake;

//[module: Sharpmake.Include("ImguiCoreLib.sharpmake.cs")]

[module: Sharpmake.Include("Common.sharpmake.cs")]
[module: Sharpmake.Include("GLFW.sharpmake.cs")]
[module: Sharpmake.Include("VulkanSDK.sharpmake.cs")]
[module: Sharpmake.Include("BuildScripts.sharpmake.cs")]
[module: Sharpmake.Include("ImguiCoreLib.sharpmake.cs")]
[module: Sharpmake.Include("Imgui.sharpmake.cs")]
[module: Sharpmake.Include("stb.sharpmake.cs")]
[module: Sharpmake.Include("nanosvg.sharpmake.cs")]
//[module: Sharpmake.Include("../ThirdParty/Sharpmake/Sharpmake.Main.Custom.sharpmake.cs")]

namespace InvirianEngine
{
    [Sharpmake.Generate]
    public class InvirianEngine : Project
    {
        public InvirianEngine()
        {
            Name = "InvirianEngine";

            AddTargets
            (
                new Target
                (
                    Platform.win64,
                    DevEnv.vs2019,
                    Optimization.Debug | Optimization.Release
                )
            );

            SourceRootPath = @"[project.SharpmakeCsPath]\..\Engine";
		}

        [Configure]
        public void ConfigureAll(Configuration conf, Target target)
        {
			conf.Output = Configuration.OutputType.Exe;
			conf.ProjectFileName = "InvirianEngine";
            conf.ProjectPath = @"[project.SharpmakeCsPath]\..\ProjectSetup\";
			conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.CPP17);


			conf.AddPrivateDependency<GLFW>(target);
			//conf.AddPrivateDependency<GLFWLib>(target);
			
			//conf.AddPrivateDependency<ImguiCoreLib>(target);
			conf.AddPrivateDependency<Imgui>(target);

			conf.AddPrivateDependency<nanosvg>(target);
			conf.AddPrivateDependency<stb>(target);
			conf.AddPrivateDependency<VulkanSDK>(target);

			conf.TargetPath = @"[project.SharpmakeCsPath]\..\Build\";
			conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings();
			conf.VcxprojUserFile.LocalDebuggerWorkingDirectory = conf.TargetPath;

			//conf.TargetLibraryPath = @"[conf.ProjectPath]\obj\[project.Name]\[target.Platform]\[target.Optimization]\[target.DevEnv]";
			//conf.IntermediatePath = @"[conf.ProjectPath]\output\[project.Name]\[target.Platform]\[target.Optimization]\[target.DevEnv]";
		}
    }

    [Sharpmake.Generate]
    public class InvirianEngineSolution : Sharpmake.Solution
    {
        public InvirianEngineSolution()
        {
            Name = "InvirianEngine";

            AddTargets(new Target(
                    Platform.win64,
                    DevEnv.vs2019,
                    Optimization.Debug | Optimization.Release
            ));

			//var batchFiles = Util.DirectoryGetFiles(@"[solution.SharpmakeCsPath]\..\", " *.bat", SearchOption.TopDirectoryOnly);
			//ExtraItems["BatchFiles"] = new Strings { batchFiles };
		}

        [Configure()]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.SolutionFileName = "InvirianEngine";
            conf.SolutionPath =@"[solution.SharpmakeCsPath]\..\ProjectSetup\";
            conf.AddProject<InvirianEngine>(target);
            conf.AddProject<GLFW>(target);
            conf.AddProject<ImguiCoreLib>(target);
            conf.AddProject<BuildScriptProject>(target);
        }
    }

    public static class Main
    {
        [Sharpmake.Main]
        public static void SharpmakeMain(Sharpmake.Arguments arguments)
        {
            //FileInfo sharpmakeFileInfo = Util.GetCurrentSharpmakeFileInfo();
            //SharpmakeGen.Globals.AbsoluteRootPath = Util.PathMakeStandard(sharpmakeFileInfo.DirectoryName);
            //Console.Write(SharpmakeGen.Globals.AbsoluteRootPath);

            KitsRootPaths.SetUseKitsRootForDevEnv(DevEnv.vs2019, KitsRootEnum.KitsRoot10, Options.Vc.General.WindowsTargetPlatformVersion.v10_0_19041_0);
            arguments.Generate<InvirianEngineSolution>();

            //SharpmakeGen.FileInfo sharpmakeFileInfo = SharpmakeGen.Util.GetCurrentSharpmakeFileInfo();
            //SharpmakeGen.Globals.AbsoluteRootPath = SharpmakeGen.Util.PathMakeStandard(sharpmakeFileInfo.DirectoryName);

            //FileInfo sharpmakeFileInfo = Util.GetCurrentSharpmakeFileInfo();
            //SharpmakeGen.Globals.AbsoluteRootPath = Util.PathMakeStandard(sharpmakeFileInfo.DirectoryName);

            //arguments.Generate<SharpmakeGen.SharpmakeSolution>();
        }
    }
}
