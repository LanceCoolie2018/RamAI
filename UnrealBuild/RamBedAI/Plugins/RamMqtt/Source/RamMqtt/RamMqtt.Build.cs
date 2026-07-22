using System.IO;
using UnrealBuildTool;

public class RamMqtt : ModuleRules
{
	public RamMqtt(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Json",
			"JsonUtilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Projects"
		});

		string RamAiRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../../../MQTTcustomPlugin"));
		string CoreInclude = Path.Combine(RamAiRoot, "core/include");
		bool bMqttNativeSupported = Target.Platform == UnrealTargetPlatform.Win64;

		if (bMqttNativeSupported)
		{
			PrivateDefinitions.Add("RAMMQTT_SUPPORTED=1");

			string VcpkgRoot = Path.Combine(RamAiRoot, "vcpkg/installed/x64-windows");

			PrivateIncludePaths.Add(CoreInclude);
			PublicIncludePaths.Add(CoreInclude);
			PublicIncludePaths.Add(Path.Combine(VcpkgRoot, "include"));
			PrivateIncludePaths.Add(Path.Combine(VcpkgRoot, "include"));

			PrivateDefinitions.Add("PAHO_MQTTPP_IMPORTS");

			PublicAdditionalLibraries.Add(Path.Combine(VcpkgRoot, "lib/paho-mqttpp3.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(VcpkgRoot, "lib/paho-mqtt3as.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(VcpkgRoot, "lib/libssl.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(VcpkgRoot, "lib/libcrypto.lib"));
			PublicSystemLibraries.Add("ws2_32.lib");

			string[] RuntimeDlls =
			{
				"paho-mqttpp3.dll",
				"paho-mqtt3as.dll",
				"libssl-3-x64.dll",
				"libcrypto-3-x64.dll"
			};

			string PluginBinariesDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Binaries/Win64"));
			string ProjectBinariesDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Binaries/Win64"));

			foreach (string DllName in RuntimeDlls)
			{
				string SourceDll = Path.Combine(VcpkgRoot, "bin", DllName);
				if (!File.Exists(SourceDll))
				{
					throw new BuildException($"Missing MQTT runtime dependency: {SourceDll}");
				}

				foreach (string DestDir in new[] { PluginBinariesDir, ProjectBinariesDir })
				{
					if (!Directory.Exists(DestDir))
					{
						Directory.CreateDirectory(DestDir);
					}

					string DestDll = Path.Combine(DestDir, DllName);
					if (!File.Exists(DestDll) || File.GetLastWriteTimeUtc(DestDll) < File.GetLastWriteTimeUtc(SourceDll))
					{
						File.Copy(SourceDll, DestDll, true);
					}

					RuntimeDependencies.Add(DestDll, StagedFileType.NonUFS);
				}
			}

			string CoreLibRelease = Path.Combine(RamAiRoot, "build/core/Release/ramai_mqtt_core.lib");
			string CoreLibDebug = Path.Combine(RamAiRoot, "build/core/Debug/ramai_mqtt_core.lib");

			if (File.Exists(CoreLibRelease))
			{
				PublicAdditionalLibraries.Add(CoreLibRelease);
			}
			else if (File.Exists(CoreLibDebug))
			{
				PublicAdditionalLibraries.Add(CoreLibDebug);
			}
			else
			{
				throw new BuildException(
					"ramai_mqtt_core.lib not found. Build MQTTcustomPlugin first: cmake --build build --config Release");
			}
		}
		else
		{
			PrivateDefinitions.Add("RAMMQTT_SUPPORTED=0");
		}
	}
}