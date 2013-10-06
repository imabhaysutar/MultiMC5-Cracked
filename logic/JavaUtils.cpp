/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "JavaUtils.h"
#include "pathutils.h"

#include <QStringList>
#include <QString>
#include <QDir>
#include <logger/QsLog.h>

JavaUtils::JavaUtils()
{

}

std::vector<java_install> JavaUtils::GetDefaultJava()
{
	std::vector<java_install> javas;
	javas.push_back(std::make_tuple("java", "unknown", "java", false));

	return javas;
}

#if WINDOWS
std::vector<java_install> JavaUtils::FindJavaFromRegistryKey(DWORD keyType, QString keyName)
{
	std::vector<java_install> javas;

	QString archType = "unknown";
	if(keyType == KEY_WOW64_64KEY) archType = "64";
	else if(keyType == KEY_WOW64_32KEY) archType = "32";

	HKEY jreKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyName.toStdString().c_str(), 0, KEY_READ | keyType | KEY_ENUMERATE_SUB_KEYS, &jreKey) == ERROR_SUCCESS)
	{
		// Read the current type version from the registry.
		// This will be used to find any key that contains the JavaHome value.
		char *value = new char[0];
		DWORD valueSz = 0;
		if (RegQueryValueExA(jreKey, "CurrentVersion", NULL, NULL, (BYTE*)value, &valueSz) == ERROR_MORE_DATA)
		{
			value = new char[valueSz];
			RegQueryValueExA(jreKey, "CurrentVersion", NULL, NULL, (BYTE*)value, &valueSz);
		}

		QString recommended = value;

		TCHAR subKeyName[255];
		DWORD subKeyNameSize, numSubKeys, retCode;

		// Get the number of subkeys
		RegQueryInfoKey(jreKey, NULL, NULL, NULL, &numSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		// Iterate until RegEnumKeyEx fails
		if(numSubKeys > 0)
		{
			for(int i = 0; i < numSubKeys; i++)
			{
				subKeyNameSize = 255;
				retCode = RegEnumKeyEx(jreKey, i, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL);
				if(retCode == ERROR_SUCCESS)
				{
					// Now open the registry key for the version that we just got.
					QString newKeyName = keyName + "\\" + subKeyName;

					HKEY newKey;
					if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, newKeyName.toStdString().c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &newKey) == ERROR_SUCCESS)
					{
						// Read the JavaHome value to find where Java is installed.
						value = new char[0];
						valueSz = 0;
						if (RegQueryValueEx(newKey, "JavaHome", NULL, NULL, (BYTE*)value, &valueSz) == ERROR_MORE_DATA)
						{
							value = new char[valueSz];
							RegQueryValueEx(newKey, "JavaHome", NULL, NULL, (BYTE*)value, &valueSz);

							javas.push_back(std::make_tuple(subKeyName, archType, QDir(PathCombine(value, "bin")).absoluteFilePath("java.exe"), (recommended == subKeyName)));
						}

						RegCloseKey(newKey);
					}
				}
			}
		}

		RegCloseKey(jreKey);
	}

	return javas;
}

std::vector<java_install> JavaUtils::FindJavaPaths()
{			
	std::vector<java_install> JRE64s = this->FindJavaFromRegistryKey(KEY_WOW64_64KEY, "SOFTWARE\\JavaSoft\\Java Runtime Environment");
	std::vector<java_install> JDK64s = this->FindJavaFromRegistryKey(KEY_WOW64_64KEY, "SOFTWARE\\JavaSoft\\Java Development Kit");
	std::vector<java_install> JRE32s = this->FindJavaFromRegistryKey(KEY_WOW64_32KEY, "SOFTWARE\\JavaSoft\\Java Runtime Environment");
	std::vector<java_install> JDK32s = this->FindJavaFromRegistryKey(KEY_WOW64_32KEY, "SOFTWARE\\JavaSoft\\Java Development Kit");

	std::vector<java_install> javas;
	javas.insert(javas.end(), JRE64s.begin(), JRE64s.end());
	javas.insert(javas.end(), JDK64s.begin(), JDK64s.end());
	javas.insert(javas.end(), JRE32s.begin(), JRE32s.end());
	javas.insert(javas.end(), JDK32s.begin(), JDK32s.end());

	if(javas.size() <= 0)
	{
		QLOG_WARN() << "Failed to find Java in the Windows registry - defaulting to \"java\"";
		return this->GetDefaultJava();
	}

	QLOG_INFO() << "Found the following Java installations (64 -> 32, JRE -> JDK): ";

	for(auto &java : javas)
	{
		QString sRec;
		if(std::get<JI_REC>(java)) sRec = "(Recommended)";
		QLOG_INFO() << std::get<JI_ID>(java) << std::get<JI_ARCH>(java) << " at " << std::get<JI_PATH>(java) << sRec;
	}

	return javas;
}
#elif OSX
std::vector<java_install> JavaUtils::FindJavaPath()
{
	QLOG_INFO() << "OS X Java detection incomplete - defaulting to \"java\"";

	return this->GetDefaultPath();
}

#elif LINUX
std::vector<java_install> JavaUtils::FindJavaPath()
{
	QLOG_INFO() << "Linux Java detection incomplete - defaulting to \"java\"";

	return this->GetDefaultPath();
}
#else
std::vector<java_install> JavaUtils::FindJavaPath()
{
	QLOG_INFO() << "Unknown operating system build - defaulting to \"java\"";

	return this->GetDefaultPath();
}
#endif