using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Sharpmake;

namespace InvirianEngine
{
	public static class CommonUtils
	{
		public static string GetEnvVariable(string variableName)
		{
			string tmp = Environment.GetEnvironmentVariable(variableName);
			if (!string.IsNullOrEmpty(tmp))
			{
				return tmp;
			}

			Console.Write("Environment variable [" + variableName + "] is not found");
			return null;
		}
	}
}
