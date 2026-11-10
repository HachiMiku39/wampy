#!/usr/bin/env python3
"""Assemble WM1A/Z service installers using the pinned upstream updater flow."""
from pathlib import Path
from io import BytesIO
import gettext, hashlib, json, re, shutil, subprocess, tarfile
root=Path.cwd(); build=root/'.arm-build'; out=build/'output'; version='1.14.7-wm1az.1'
payload=build/'package/payload'; payload.parent.mkdir(parents=True,exist_ok=True)
shutil.copytree(build/'base/payload',payload)
newplayer=out/'wampy'; assert newplayer.read_bytes()[:4]==b'\x7fELF'
shutil.copy2(newplayer,payload/'wampy'); (payload/'wampy').chmod(0o755)
(payload/'product_info').write_text('wampy, version '+version+'\n')
# Rebuild translations and their glyph lists while preserving the archive layout.
replacements={}
for po in (root/'tl/po').glob('*.po'):
    mo=build/'package'/f'{po.stem}.mo'
    subprocess.run(['msgfmt','--check','-o',str(mo),str(po)],check=True)
    with mo.open('rb') as f: catalog=gettext.GNUTranslations(f)._catalog
    chars=sorted(set(''.join(str(v) for v in catalog.values())))
    replacements[po.stem]={'wampy.mo':mo.read_bytes(),'range':''.join(chars).encode()}
oldlocale=build/'base/payload/tl.tar.gz'
with tarfile.open(oldlocale,'r:*') as source,tarfile.open(payload/'tl.tar.gz','w:gz',format=tarfile.GNU_FORMAT) as target:
    for member in source:
        data=source.extractfile(member).read() if member.isfile() else None
        for language,files in replacements.items():
            for name,content in files.items():
                if member.name.endswith(language+'/LC_MESSAGES/'+name): data=content;member.size=len(data)
        target.addfile(member,BytesIO(data) if data is not None else None)
userdata=build/'package/userdata.tar.gz'
with tarfile.open(userdata,'w',format=tarfile.GNU_FORMAT) as archive:
    for p in sorted(payload.iterdir()): archive.add(p,arcname=p.name)
upg=root/'nw-installer/tools/upgtool/upgtools/upgtool'
entries=[str(build/'base/entries'/f'{n}.bin') for n in range(9)]
# The first nine service-wrapper entries are unchanged from upstream 1.14.6.
engine_hashes={str(n):hashlib.sha256(Path(p).read_bytes()).hexdigest() for n,p in enumerate(entries)}
full=out/f'Wampy-WM1AZ-WalkmanOne-{version}.UPG'
subprocess.run([str(upg),'-m','nw-wm1a','-w','-c',str(full),*entries,str(userdata)],check=True)
verify=build/'package/verify';verify.mkdir()
subprocess.run([str(upg),'-m','nw-wm1a','-w','-e','-o',str(verify)+'/',str(full)],check=True)
for n,p in enumerate(entries): assert (verify/f'{n}.bin').read_bytes()==Path(p).read_bytes()
assert (verify/'9.bin').read_bytes()[:userdata.stat().st_size]==userdata.read_bytes()
# Compile the original Windows wizard with only the WM1A/Z selector enabled.
windows=build/'package/windows';windows.mkdir()
for name in ['device.bmp','explorer.bmp','dvdfunc.nsh']:
    shutil.copy2(root/'nw-installer/installer/windows'/name,windows/name)
script=(root/'nw-installer/installer/windows/script.nsh').read_text()
start=script.index('    ${If} $HasWalkmanOne == 1',script.index('Section reviewPageLeave'))
end=script.index('    ${If} $SelectedAction == 1',start)
script=script[:start]+'''    ${If} $HasWalkmanOne == 1
        File "./walkmanOne/NW_WM_FW.UPG"
    ${Else}
        ${If} $SelectedDevice == "wm1az"
            File "./nw-wm1a/NW_WM_FW.UPG"
        ${Else}
            MessageBox mb_iconstop "Invalid model selected"
            Quit
        ${EndIf}
    ${EndIf}

'''+script[end:]
(windows/'script.nsh').write_text(script)
for folder,flags in [('walkmanOne',['-w']),('nw-wm1a',[])]:
    (windows/folder).mkdir()
    subprocess.run([str(upg),'-m','nw-wm1a',*flags,'-c',str(windows/folder/'NW_WM_FW.UPG'),*entries],check=True)
# Keep the upstream uninstaller data and exact original scsitool.
def download(url,path,expected):
    subprocess.run(['curl','-fL','--retry','2','--max-time','180',url,'-o',str(path)],check=True)
    assert hashlib.sha256(path.read_bytes()).hexdigest()==expected,(path,'checksum mismatch')
undo=build/'package/upstream-uninstaller.tar.gz'
download('https://github.com/unknown321/wampy/releases/download/v1.14.6/walkmanOne.uninstaller.tar.gz',undo,'71ada75cca6cad4d20efc986b95228554b06471853e36c2784b27628c800db95')
with tarfile.open(undo,'r:*') as archive:
    member=next(m for m in archive if m.name.endswith('NW_WM_FW.UPG'))
    undo_upg=build/'package/upstream-uninstaller.UPG';undo_upg.write_bytes(archive.extractfile(member).read())
undo_dir=build/'package/undo';undo_dir.mkdir()
subprocess.run([str(upg),'-m','nw-wm1a','-w','-e','-o',str(undo_dir)+'/',str(undo_upg)],check=True)
shutil.copy2(undo_dir/'9.bin',build/'package/userdata.uninstaller.tar.gz')
shutil.copy2(undo_upg,out/f'Wampy-WM1AZ-WalkmanOne-uninstaller.UPG')
exe=build/'package/upstream.exe'
download('https://github.com/unknown321/wampy/releases/download/v1.14.6/wampy.2026-06-16.v1.14.6.exe',exe,'bd832fb71f48b89929f88cb7bb01d61f95562fb08bb7e0de032af247a6cec2c6')
subprocess.run(['7z','e','-y','-o'+str(windows),str(exe),'$PLUGINSDIR/scsitool-nwz-v27.exe'],check=True)
assert hashlib.sha256((windows/'scsitool-nwz-v27.exe').read_bytes()).hexdigest()=='71893c46299d0465cad41c8dcd4fcf630e003e578648b32512760ef4a43e284a'
(windows/'LICENSE.txt').write_text((payload/'LICENSE').read_text()+'\n'+(payload/'LICENSE_3rdparty').read_text()+'\n'+(root/'nw-installer/LICENSE').read_text())
installer=out/f'Wampy-WM1AZ-{version}.exe'
subprocess.run(['makensis','-DAPPNAME=Wampy-WM1AZ','-DOUTFILE='+str(installer),'-DA50=0','-DA40=0','-DA30=0','-DA50Z=0','-DWM1AZ=1','-DZX300=0','-DDMPZ1=0','-DA40MOD_ONLY=0','-DA30MOD_ONLY=0','script.nsh'],cwd=windows,check=True)
manifest={'version':version,'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'upstream_base':'unknown321/wampy v1.14.6','engine_entries_unchanged':engine_hashes,'target':'first-generation WM1A/Z, Walkman One','hardware_tested':False,'device_upgrade_performed':False,'files':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file()}}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
(out/'INSTALL.txt').write_text('Wampy WM1A/Z '+version+' development build\n\nFor the user-confirmed Walkman One device: run the EXE in Windows, select Install, NW-WM1A/Z, and YES for Walkman One. Select internal WALKMAN storage. Follow the upstream wizard instructions and allow both updater restarts to finish. Toggle HOLD after the Sony database scan.\n\nThis package has completed ARM build and static package validation only. No device has been flashed or hardware playback/display validation performed. Back up the device before an installation test.\n\nThe standalone UPG uses Walkman One encryption and must not be used as a stock-firmware UPG. An upstream Walkman One uninstaller is included.\n')
print('Created installer:',installer)
