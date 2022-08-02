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
    public class VulkanSDK : Project
    {
		public VulkanSDK()
		{
			Name = "VulkanSDK";
			IsFileNameToLower = false;

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
			conf.Output = Configuration.OutputType.None;
			string VulkanSDKPath = CommonUtils.GetEnvVariable("VULKAN_SDK");
			//Console.Write("Vulkan sdk path: " + VulkanSDKPath);

			conf.IncludePaths.Add(VulkanSDKPath + "/Include");
			conf.LibraryPaths.Add(VulkanSDKPath + "/Lib");
			conf.LibraryFiles.Add(@"vulkan-1.lib");
		}
    }
}
