#!/usr/bin/python

# This script creates a new Half-Life: VR directory from a vanilla Half-Life installation.
# Must be run from the 'src' directory within a freshly (and completely) cloned Half-Life: VR repository (https://github.com/maxvollmer/Half-Life-VR).
# Run like so: python CreateHLVRFromHL.py -h <hlpath>
# where <hlpath> is the full path to your Half-Life installation.
# NOTE: This just prepares the folder.
# NOTE: You still need to build client.dll, hl.dll, and HLVRConfig.exe for the mod to actually work.

# THIS SCRIPT IS FOR DEVELOPERS WHO WANT TO BUILD THE MOD
# IT IS NOT INTENDED FOR PLAYERS
# USE AT OWN RISK

import sys, getopt, os, shutil, pathlib

dlls_to_copy = [
"cl_dll/openvr/openvr_api.dll",

"cl_dll/EasyHook/bin/EasyHook32.dll",
"cl_dll/fmod/lib/x86/fmod.dll",
"cl_dll/fmod/lib/x86/fmodL.dll",

"packages/Microsoft.Experimental.Collections.1.0.6-e190117-3/lib/netstandard2.0/Microsoft.Experimental.Collections.dll",
"packages/MarkdownSharp.2.0.5/lib/net40/MarkdownSharp.dll",
"packages/Newtonsoft.Json.13.0.1/lib/net45/Newtonsoft.Json.dll",
"packages/HtmlToXamlConverter.1.0.5727.24510/lib/net45/HtmlToXamlConverter.dll"
]

folders_to_copy = [
'../art/sound',
'../art/textures',
'../art/sprites',
'../art/models',

'../game/actions',
'../game/resource',
'../game/fonts'
]

def main(argv):
    script_location = os.path.dirname(pathlib.Path(__file__).absolute())

    if not os.path.basename(script_location) == "src" or not os.path.exists(os.path.join(script_location, 'HLVR.sln')):
        print('Error: Unexpected script location. Script must be run from the "src" folder in a fully checked out Half-Life: VR repository (https://github.com/maxvollmer/Half-Life-VR).')
        sys.exit(2)

    if not os.path.exists(os.path.join(script_location, 'packages')):
        print('Error: "packages" folder not found in "src" folder. Please install all NuGet packages required by HLVRConfig before continuing.')
        sys.exit(2)

    for dll in dlls_to_copy:
        if not os.path.exists(os.path.join(script_location, dll)):
            print('Error: Library "'+dll+'" not found in repository. Make sure you have a fully checked out Half-Life: VR repository (https://github.com/maxvollmer/Half-Life-VR), AND ensure you have installed all NuGet packages required by HLVRConfig.')
            sys.exit(2)

    for folder in folders_to_copy:
        if not os.path.exists(os.path.join(script_location, folder)):
            print('Error: Folder "'+folder+'" not found in repository. Make sure you have a fully checked out Half-Life: VR repository (https://github.com/maxvollmer/Half-Life-VR).')
            sys.exit(2)

    hlpath = ''

    try:
        opts, args = getopt.getopt(argv, "h:",["hlpath="])
        for opt, arg in opts:
            if opt in ("-h", "--hlpath"):
                hlpath = arg
    except getopt.GetoptError:
        print('Error: Please provide a valid path to Half-Life like so: "CreateHLVRFromHL.py -h <hlpath>"')
        sys.exit(2)

    if not os.path.exists(hlpath):
        print('Error: Path doesn\'t exist. Please provide a valid path to Half-Life like so: "CreateHLVRFromHL.py -h <hlpath>"')
        sys.exit(2)

    if not os.path.basename(hlpath) == "Half-Life":
        print('Error: Expected folder name "Half-Life", "'+os.path.basename(hlpath)+'" found instead. Please provide a valid path to Half-Life like so: "CreateHLVRFromHL.py -h <hlpath>"')
        sys.exit(2)

    if not os.path.exists(os.path.join(hlpath,"hl.exe")):
        print('Error: No hl.exe found in Half-Life folder. Please provide a valid path to Half-Life like so: "CreateHLVRFromHL.py -h <hlpath>"')
        sys.exit(2)

    hlvrpath = hlpath + ' VR'

    if os.path.exists(hlvrpath):
        print('Error: "' + hlvrpath + '" already exists, aborting.')
        sys.exit(2)


    # copy Half-Life
    print('Creating Half-Life: VR folder at "' + hlvrpath + '" from Half-Life folder at "' + hlpath + '". Please wait, this can take a while.')
    shutil.copytree(hlpath, hlvrpath)

    # copy dlls to game folder
    for dll in dlls_to_copy:
        print('Copying "'+os.path.join(script_location, dll)+'".')
        shutil.copy(os.path.join(script_location, dll), hlvrpath)

    # copy folders to valve/
    for folder in folders_to_copy:
        print('Copying "'+os.path.join(script_location, folder)+'" to "'+os.path.join(hlvrpath, 'valve', os.path.basename(folder))+'".')
        shutil.copytree(os.path.join(script_location, folder), os.path.join(hlvrpath, 'valve', os.path.basename(folder)), dirs_exist_ok=True)

    # delete obsolete valve_hd folder (HD models are handled differently in HL:VR)
    if os.path.exists(os.path.join(hlvrpath, 'valve_hd')):
        print('Removing obsolete folder "valve_hd".')
        shutil.rmtree(os.path.join(hlvrpath, 'valve_hd'))

    # additional stuff
    print('Copying licenses, readme, liblist.gam, and game icon.')
    shutil.copytree(os.path.join(script_location, '../docs/licenses'), os.path.join(hlvrpath, 'licenses'), dirs_exist_ok=True)
    shutil.copy(os.path.join(script_location, '../game/README.md'), hlvrpath)
    shutil.copy(os.path.join(script_location, '../game/liblist.gam'), os.path.join(hlvrpath, 'valve', 'liblist.gam'))
    shutil.copy(os.path.join(script_location, '../art/game_icon.ico'), os.path.join(hlvrpath, 'valve', 'game.ico'))

    print('All done! New Half-Life: VR folder created at "'+hlvrpath+'". NOTE: You still need to build client.dll, hl.dll, and HLVRConfig.exe for the mod to work.')

if __name__ == "__main__":
    main(sys.argv[1:])
