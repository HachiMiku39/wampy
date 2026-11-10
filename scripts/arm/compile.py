#!/usr/bin/env python3
"""Compile the production application using the pinned GCC4.9 ARM Linux SDK."""
from pathlib import Path
import concurrent.futures, json, os, re, shlex, subprocess
root=Path.cwd(); build=root/'.arm-build'; env=json.loads((build/'environment.json').read_text())
source=(root/'CMakeLists.txt').read_text().split('add_executable(${PROJECT_NAME}',1)[1].split(')',1)[0]
source='\n'.join(line.split('#',1)[0] for line in source.splitlines())
source=source.replace('${IMGUI_DIR}','libs/imgui').replace('${PROTOBUF_SOURCE}','server/armv5-unknown-linux-gnueabihf/command.pb.cc').replace('${CONNECTOR_SOURCE}','src/connector/hagoromo.cpp server/armv5-unknown-linux-gnueabihf/command_names.cpp')
files=[p for p in source.split() if p.endswith(('.c','.cc','.cpp'))]
(root/'src/Version.h').write_text('#pragma once\n#define SOFTWARE_VERSION "1.14.7-wm1az.1"\n')
includes=['src','src/util','libs/imgui','libs/imgui/backends','libs/glfw/include','libs','libs/glad/include','libs/glad/include/KHR','libs/glm','libs/implot','libs/mINI/src','libs/zlib','libs/ImageMagick','libs/ImageMagick/Magick++/lib','libs/protobuf/src','server/armv5-unknown-linux-gnueabihf','pstserver','sound_service_fw',str(build/'icu/source/common'),str(build/'icu/source/i18n'),str(build/'alsa/include'),str(build/'sqlite'),str(build/'include')]
flags=['-O2','-g','-fno-omit-frame-pointer','-DMAGICKCORE_HDRI_ENABLE=1','-DMAGICKCORE_QUANTUM_DEPTH=32','-DIMGUI_USER_CONFIG="../../src/imgui.config.h"','-D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1','-D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2','-D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4']+['-I'+p for p in includes]
objects=[]; commands=[]
for p in files:
    obj=build/'objects'/(p+'.o');obj.parent.mkdir(parents=True,exist_ok=True);objects.append(str(obj))
    cmd=[env['cc'] if p.endswith('.c') else env['compiler'],'-std=gnu99' if p.endswith('.c') else '-std=c++11',*flags,'-c',p,'-o',str(obj)]
    commands.append((p,cmd))
(build/'logs/compile_commands.json').write_text(json.dumps([{'directory':str(root),'file':p,'arguments':cmd} for p,cmd in commands],indent=2))
def compile_one(item):
    p,cmd=item
    print('Compiling',p,flush=True)
    result=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    (build/'logs'/(p.replace('/','_')+'.log')).write_text(result.stdout)
    if result.returncode: print(result.stdout,flush=True);raise RuntimeError(p)
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(compile_one,commands))
lib=build/'sysroot/lib';out=build/'output/wampy'
cmd=[env['compiler'],*objects,str(build/'glfw/src/libglfw3.a'),str(build/'zlib/libz.a'),'-L'+str(lib),'-Wl,-rpath-link,'+str(lib),'-Wl,-rpath,/system/vendor/unknown321/lib:/system/vendor/sony/lib:/lib','-lMagick++-7.Q8HDRI','-lMagickWand-7.Q8HDRI','-lMagickCore-7.Q8HDRI','-lprotobuf','-ljpeg','-lshine','-lsqlite3','-licui18n','-licuuc','-licudata','-lMali_linux','-lasound','-lpthread','-ldl','-lm','-lrt','-o',str(out)]
(build/'logs/link-command.txt').write_text(shlex.join(cmd)+'\n');subprocess.run(cmd,check=True)
subprocess.run([env['prefix']+'strip','--strip-debug',str(out)],check=True)
for name,args in [('elf-header',['-h']),('elf-dependencies',['-d']),('elf-versions',['-V']),('elf-attributes',['-A'])]:
    result=subprocess.check_output([env['prefix']+'readelf',*args,str(out)],text=True);(build/'output'/(name+'.txt')).write_text(result)
(build/'output/source-commit.txt').write_text(subprocess.check_output(['git','rev-parse','HEAD'],text=True))
