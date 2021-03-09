// Probably makes sense to make this a python script...

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <unordered_map>

int main(int argc, char* argv[])
{
    if (argc < 2)
        return 1;

    std::unordered_map<std::string, std::string> dllnames = {
        {"EasyHook32.dll", "vr/ezhok32.dll"},
        {"openvr_api.dll", "vr/opnvrpi.dll"},
        {"fmod.dll", "vr/f.dll"},
        {"fmodL.dll", "vr/fL.dll"},
        {"steam_api.dll", "vr/stmapi.dll"},
    };

    std::vector<std::string> fixeddlls;
    std::vector<std::string> notfixeddlls;

    std::fstream file(argv[1], std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        std::streampos size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> bytes(size);
        file.read(bytes.data(), size);

        for (auto& [originaldll, newdll] : dllnames)
        {
            auto originaldllnamepos = std::search(bytes.begin(), bytes.end(), std::begin(originaldll), std::end(originaldll));
            if (originaldllnamepos != bytes.end())
            {
                std::memcpy(bytes.data() + std::distance(bytes.begin(), originaldllnamepos), newdll.data(), newdll.length());
                fixeddlls.push_back(originaldll);
            }
            else
            {
                notfixeddlls.push_back(originaldll);
            }
        }

        file.seekg(0, std::ios::beg);
        file.write(bytes.data(), size);
        file.flush();
    }

    file.close();

    std::cout << "DLLIncludeFixer (" << argv[1] << "): " << std::endl;

    for (auto& fixeddll : fixeddlls)
    {
        std::cout << "Fixed " << fixeddll << "." << std::endl;

    }

    for (auto& notfixeddll : notfixeddlls)
    {
        std::cout << "Didn't fix " << notfixeddll << "." << std::endl;
    }

    std::cout << std::flush;

    return (notfixeddlls.size() == 1) ? 0 : 1;
}
