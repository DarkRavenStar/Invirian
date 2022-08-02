using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Sharpmake;

namespace InvirianEngine
{
    [Sharpmake.Export]
    public class GLFW : Project
    {
		public GLFW()
		{
			Name = "GLFW";
			IsFileNameToLower = false;
			//SourceRootPath = @"[project.SharpmakeCsPath]\..\ThirdParty\glfw\";
			
			SourceFiles.Add
			(
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\include\GLFW\glfw3.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\include\GLFW\glfw3native.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\internal.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\platform.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\mappings.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\context.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\init.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\input.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\monitor.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\platform.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\vulkan.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\window.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\egl_context.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\osmesa_context.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_platform.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_joystick.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_init.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_monitor.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_window.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_joystick.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_time.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_thread.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_module.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_time.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_thread.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_platform.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_joystick.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_init.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_joystick.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_monitor.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_window.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\wgl_context.c"
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
			conf.SolutionFolder = "Dependency";

			//conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\include\");//, @"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\");
			conf.ProjectPath = @"[project.SharpmakeCsPath]\..\ThirdParty\glfw\bin\src\";
			//conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\bin\src\[target.Optimization]\");
			//conf.LibraryFiles.Add(@"glfw3.lib");

			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\include\");
			conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\bin\src\[target.Optimization]\");
			conf.LibraryFiles.Add(@"glfw3.lib");
			//conf.LibraryFiles.Add(@"glfw3_mt.lib");

			//conf.Output = target.OutputType == OutputType.Lib ? Configuration.OutputType.Lib : Configuration.OutputType.Dll;
		}
	}

	[Sharpmake.Export]
	public class GLFWLib : Project
	{
		public GLFWLib()
		{
			Name = "GLFWLib";
			IsFileNameToLower = false;

			/*
			SourceFiles.Add
			(
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\include\GLFW\glfw3.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\include\GLFW\glfw3native.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\internal.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\platform.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\mappings.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\context.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\init.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\input.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\monitor.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\platform.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\vulkan.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\window.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\egl_context.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\osmesa_context.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_platform.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_joystick.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_init.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_monitor.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_window.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\null_joystick.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_time.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_thread.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_module.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_time.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_thread.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_platform.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_joystick.h",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_init.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_joystick.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_monitor.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\win32_window.c",
				@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\src\wgl_context.c"
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
			//conf.AddPrivateDependency<GLFW>(target);

			//conf.Output = Configuration.OutputType.Lib;
			//conf.Output = Configuration.OutputType.Utility;
			
			
			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\include\");
			conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\glfw\bin\src\[target.Optimization]\");
			conf.LibraryFiles.Add(@"glfw3.lib");
			
			/*
			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\glfwcompiled\include\");
			conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\glfwcompiled\lib-vc2019\");
			conf.LibraryFiles.Add(@"glfw3.lib");
			*/
		}
	}
}
