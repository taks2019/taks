param(
    [ValidateSet("menu","check","build","install","tests","studio","copy-images","logs","ground","screenshot")]
    [string]$Mode = "menu",
    [string]$DeviceSerial = "",
    [string]$SourceZip = ""
)
# TAKS World Windows -> native Android. No Unity, no Godot, no HTML.
# Does not delete user projects, existing APKs, phone app data or accepted artwork.
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Tools = Join-Path $Root ".tools"
$Out = Join-Path $Root "output"
$Venv = Join-Path $Root ".venv"
$ApprovedName = "WONDERWERK_APPROVED_WIDE_NO_TEXT.png"
$ApprovedHash = "9369d3668b69424d31132e70accbfdb76282d7d9b3f3adfaa51c7c3ca1ea7bc8"
$ApprovedPath = Join-Path $Root "app\src\main\assets\menu\$ApprovedName"
$TestApk = Join-Path $Out "TAKS_WORLD_PC_TESZT.apk"
$RequiredModels = @(
    "rock_07.tkm","rock_07.png",
    "namaqualand_boulders_01.tkm","namaqualand_boulders_01.png",
    "namaqualand_cliff_01.tkm","namaqualand_cliff_01.png",
    "grass_bermuda_01.tkm","grass_bermuda_01.png",
    "fern_02.tkm","fern_02.png",
    "makehuman_rigged.tks","makehuman_rigged.png",
    "ground_sandy_gravel.png","wall_cliff_side.png"
)
function Say([string]$Message){ Write-Host $Message -ForegroundColor Cyan }
function Fail([string]$Message){ throw $Message }
function Run([string]$Executable,[string[]]$Arguments){
    & $Executable @Arguments
    if($LASTEXITCODE -ne 0){ Fail "A parancs hibaval kilepett ($LASTEXITCODE): $Executable $($Arguments -join ' ')" }
}
function Find-Command([string]$Name){
    $value = Get-Command $Name -ErrorAction SilentlyContinue
    if($null -ne $value){ return $value.Source }
    return $null
}
function Sdk-Location {
    $candidates = New-Object "System.Collections.Generic.List[string]"
    if($env:ANDROID_HOME){$candidates.Add($env:ANDROID_HOME)}
    if($env:ANDROID_SDK_ROOT){$candidates.Add($env:ANDROID_SDK_ROOT)}
    if($env:LOCALAPPDATA){$candidates.Add((Join-Path $env:LOCALAPPDATA "Android\Sdk"))}
    foreach($path in $candidates){
        if(Test-Path (Join-Path $path "platform-tools\adb.exe")){return $path}
    }
    return $null
}
function Need-Sdk {
    $sdk=Sdk-Location
    if(-not $sdk){Fail "Az Android SDK hianyzik. Android Studio -> More Actions -> SDK Manager. Telepitsd: API 35, Build Tools 35, NDK 27.2.12479018, CMake 3.22.1, Platform Tools."}
    $env:ANDROID_HOME=$sdk
    $env:ANDROID_SDK_ROOT=$sdk
    if(-not (Test-Path (Join-Path $sdk "platforms\android-35\android.jar"))){
        Fail "Az Android SDK 35 hianyzik. Android Studio -> SDK Manager -> Android 15 (API 35)."
    }
    if(-not (Test-Path (Join-Path $sdk "ndk\27.2.12479018\source.properties"))){
        Fail "A projekt NDK 27.2.12479018 verziot ker. Android Studio -> SDK Tools -> Show Package Details -> NDK 27.2.12479018."
    }
    if(-not (Test-Path (Join-Path $sdk "cmake\3.22.1\bin\cmake.exe"))){
        Fail "CMake 3.22.1 hianyzik. Android Studio -> SDK Tools -> Show Package Details -> CMake 3.22.1."
    }
    return $sdk
}
function Need-Java {
    $javaHomes=@()
    if($env:JAVA_HOME){$javaHomes+= $env:JAVA_HOME}
    $javaHomes+= "C:\Program Files\Android\Android Studio\jbr"
    foreach($home in $javaHomes){
        if(Test-Path (Join-Path $home "bin\java.exe")){
            $env:JAVA_HOME=$home
            $env:PATH=(Join-Path $home "bin")+";"+$env:PATH
            return
        }
    }
    $java=Find-Command "java.exe"
    if(-not $java){ Fail "Java hianyzik: telepitsd az ingyenes Android Studiot (JDK 17 vagy ujabb)." }
}
function Save-SdkPath([string]$Sdk){
    $property=Join-Path $Root "local.properties"
    if(Test-Path $property){ return } # Never overwrite an existing local setup.
    $value=$Sdk.Replace("\","/")
    $utf8=New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($property,"sdk.dir=$value" + [Environment]::NewLine,$utf8)
}
function Python-Launcher {
    $py=Find-Command "py.exe"
    if($py){
        & $py "-3.11" "-c" "import sys; assert sys.version_info >= (3,11)"
        if($LASTEXITCODE -eq 0){return @{Exe=$py;Prefix=@("-3.11")}}
    }
    $python=Find-Command "python.exe"
    if($python){
        & $python "-c" "import sys; assert sys.version_info >= (3,11)"
        if($LASTEXITCODE -eq 0){return @{Exe=$python;Prefix=@()}}
    }
    Fail "Python 3.11+ hianyzik. Telepitsd a python.org-rol, pip es venv opciokkal."
}
function Need-PythonVenv {
    $exe=Join-Path $Venv "Scripts\python.exe"
    if(-not (Test-Path $exe)){
        $py=Python-Launcher
        Say "Elkulonitett helyi Python kornyezet letrehozasa..."
        $arguments=@($py.Prefix)+@("-m","venv",$Venv)
        Run $py.Exe $arguments
    }
    # Missing Python packages are normal on the first build, not a fatal
    # PowerShell stderr exception (Windows PowerShell 5.1 treats it as one
    # when ErrorActionPreference=Stop).
    $savedErrorPreference=$ErrorActionPreference
    try{
        $ErrorActionPreference="Continue"
        & $exe "-c" "import numpy, requests, trimesh, PIL, scipy" 2>$null
        $packagesReady=($LASTEXITCODE -eq 0)
    }finally{
        $ErrorActionPreference=$savedErrorPreference
    }
    if(-not $packagesReady){
        Say "Elso alkalom: a valodi 3D modellek importjanak ingyenes fuggosegei..."
        Run $exe @("-m","pip","install","--disable-pip-version-check","numpy>=1.26,<3","Pillow>=10","requests>=2.31","trimesh>=4.10,<5","scipy>=1.11,<2")
    }
    return $exe
}
function Need-Gradle {
    $exe=Join-Path $Tools "gradle-8.9\bin\gradle.bat"
    if(Test-Path $exe){return $exe}
    $system=Find-Command "gradle.bat"
    if($system){
        $versionText=((& $system "--version" 2>&1) | Out-String)
        if($versionText -match 'Gradle 8\.9\b'){return $system}
    }
    Say "Az elso fordítashoz letoltom a hivatalos Gradle 8.9-et (~130 MB); a gepeden marad."
    New-Item -ItemType Directory -Force $Tools | Out-Null
    $url="https://services.gradle.org/distributions/gradle-8.9-bin.zip"
    $checksum=((Invoke-WebRequest -UseBasicParsing -Uri ($url+".sha256")).Content | Out-String)
    $match=[regex]::Match($checksum,'[A-Fa-f0-9]{64}')
    if(-not $match.Success){Fail "Nem sikerult megszerezni a Gradle eredeti SHA256 ellenorzoosszeget."}
    $zip=Join-Path $Tools "gradle-8.9-bin.zip"
    if(-not (Test-Path $zip)){Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $zip}
    $actual=(Get-FileHash -Algorithm SHA256 -LiteralPath $zip).Hash
    if($actual -ine $match.Value){ Fail "A Gradle letoltes SHA256 erteke hibas. A ZIP-et nem futtatom." }
    Expand-Archive -LiteralPath $zip -DestinationPath $Tools -Force
    if(-not (Test-Path $exe)){Fail "A Gradle kicsomagolasa nem sikerult."}
    return $exe
}
function Prepare-ApprovedMenu {
    if(Test-Path $ApprovedPath){
        $current=(Get-FileHash -Algorithm SHA256 -LiteralPath $ApprovedPath).Hash
        if($current -ieq $ApprovedHash){Say "Az eredeti elfogadott fomenukep valtozatlan.";return}
        Fail "A letezo menukep nem az elfogadott eredeti! Nem irom felul: $ApprovedPath"
    }
    Say "Elso alkalom: az elfogadott menuszintu kep kinyerese a kiadott TAKS APK-bol..."
    New-Item -ItemType Directory -Force $Tools | Out-Null
    $archive=Join-Path $Tools "original-approved-menu-source.apk"
    $url="https://github.com/taks2019/taks/releases/download/taks-world-0.16.5-native-49/TAKS_World_Android_KIPROBALO.apk"
    if(-not (Test-Path $archive)){Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $archive}
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[System.IO.Compression.ZipFile]::OpenRead($archive)
    try{
        $entry=$zip.GetEntry("assets/menu/$ApprovedName")
        if($null -eq $entry){Fail "A forras APK-ban nincs meg az eredeti elfogadott fomenukep."}
        $dir=Split-Path -Parent $ApprovedPath
        New-Item -ItemType Directory -Force $dir | Out-Null
        $source=$entry.Open()
        $output=[System.IO.File]::Create($ApprovedPath)
        try{$source.CopyTo($output)}finally{$output.Dispose();$source.Dispose()}
    }finally{$zip.Dispose()}
    $verified=(Get-FileHash -Algorithm SHA256 -LiteralPath $ApprovedPath).Hash
    if($verified -ine $ApprovedHash){
        Remove-Item -LiteralPath $ApprovedPath -Force
        Fail "Nem az eredeti menukepet kaptuk: SHA256 elteres."
    }
    Say "Az elfogadott kep merete es hash-e igazolt, az eredeti forras nem valtozott."
}
function Prepare-Models {
    $modelFolder=Join-Path $Root "app\src\main\assets\models"
    $missing=@($RequiredModels | Where-Object { -not (Test-Path (Join-Path $modelFolder $_)) })
    if($missing.Count -eq 0){Say "A korabban hasznalt modellek mar el vannak keszitve.";return}
    Say "Elso alkalom: a korabbi tesztekhez hasznalt ingyenes CC0 modellek importja."
    Say "FONTOS: ezek meg NEM jovahagyott, tortenetileg hiteles Wonderwerk-jatekmodellek."
    $python=Need-PythonVenv
    foreach($script in @("import_cc0_rocks.py","import_cc0_foliage.py","import_cc0_rigged_human.py")){
        Run $python @((Join-Path $Root "scripts\$script"))
    }
    $missing=@($RequiredModels | Where-Object { -not (Test-Path (Join-Path $modelFolder $_)) })
    if($missing.Count -gt 0){Fail "A 3D modellek importja utan meg hianyzik: $($missing -join ', ')"}
}
function Prepare-ApprovedGround {
    $assetFolder=Join-Path $Root "app\src\main\assets\approved_ground"
    $manifestPath=Join-Path $assetFolder "approved_ground_manifest.json"
    $sourceCandidate=$SourceZip
    if(-not $sourceCandidate){
        $sourceCandidate=Join-Path $Root "approved_source\SuperImage.zip"
    }
    $hasSource=(Test-Path -LiteralPath $sourceCandidate -PathType Leaf)
    # FILE PICKER: the GUI menu's 8th item can find the user's ZIP
    # without needing any PowerShell path typing or Git commands.
    if(-not $hasSource -and $Mode -eq "ground" -and -not $SourceZip){
        Add-Type -AssemblyName System.Windows.Forms
        $fileDialog=New-Object System.Windows.Forms.OpenFileDialog
        $fileDialog.Title="TAKS WORLD - válaszd ki az eredeti SuperImage.zip fájlt"
        $fileDialog.Filter="SuperImage képcsomag (*.zip)|*.zip"
        $fileDialog.CheckFileExists=$true
        if($fileDialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK){
            $sourceCandidate=$fileDialog.FileName
            $hasSource=$true
        }
    }
    if($hasSource){
        Say "Hat eredeti SuperImage-talaj automatikus eloallitasa a teszt APK-hoz."
        $python=Need-PythonVenv
        Run $python @((Join-Path $Root "scripts\bake_approved_ground.py"),
                     "--source",$sourceCandidate,"--out",$assetFolder)
    }else{
        Say "Nem kaptam forras ZIP-et; ellenorzom, van-e mar korabban elkeszitett talajkeszlet."
    }
    $expected=@("dry.png","red.png","stone.png","sand.png","mud.png","transition.png")
    $missing=@($expected | Where-Object { -not (Test-Path (Join-Path $assetFolder $_)) })
    if($missing.Count -gt 0 -or -not (Test-Path $manifestPath)){
        Fail "A korabbi barna jatekteret NEM epitem ujra! Hianyzik a hiteles SuperImage.zip. Masold ide: approved_source\SuperImage.zip, vagy inditsd: PC_TAKS_WORLD.cmd -Mode build -SourceZip 'D:\Kepek\SuperImage.zip'"
    }
    $manifest=Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $known="71600a427e5d17cc0334a07ca9a0a827ad747f2caaed326efaa2603c30d55660"
    if($manifest.source_sha256 -ne $known){
        Fail "Nem az elfogadott eredeti kepbol szarmazik a helyi talajkeszlet."
    }
    if(@($manifest.tiles).Count -ne 6){Fail "A talajmanifest hiányos: pontosan hat jóváhagyott minta szükséges."}
    foreach($record in $manifest.tiles){
        if($expected -notcontains $record.file){Fail "Ismeretlen talajnev a manifestben."}
        $file=Join-Path $assetFolder $record.file
        if(-not (Test-Path -LiteralPath $file)){Fail "Hianyzo talaj: $file"}
        if((Get-FileHash -Algorithm SHA256 -LiteralPath $file).Hash -ine $record.sha256){
            Fail "A talajkep hash-e megvaltozott. Nem irom felul: $file"
        }
    }
    Say "A teszt APK hat igazolt, fotoalapu talajmasolatot tartalmaz, az eredeti ZIP valtozatlan."
}
function Build-Android {
    Say "TAKS WORLD: PC -> Android teszt-APK epitese."
    $sdk=Need-Sdk
    Need-Java
    Save-SdkPath $sdk
    Prepare-ApprovedMenu
    Prepare-Models
    Prepare-ApprovedGround
    $gradle=Need-Gradle
    Push-Location $Root
    try{Run $gradle @("--no-daemon",":app:assembleDebug","--stacktrace")}
    finally{Pop-Location}
    $built=Join-Path $Root "app\build\outputs\apk\debug\app-debug.apk"
    if(-not (Test-Path $built)){Fail "A Gradle nem hozta letre az app-debug.apk fajlt."}
    # DO NOT announce another 'fixed' APK if the approved scene and six
    # photographic surfaces were silently absent from Gradle's output.
    $verifyPython=Need-PythonVenv
    Run $verifyPython @((Join-Path $Root "scripts\check_apk_ground_pack.py"),"--apk",$built)
    New-Item -ItemType Directory -Force $Out | Out-Null
    Copy-Item -LiteralPath $built -Destination $TestApk -Force
    Say "KESZ - TESZT APK: $TestApk"
    Write-Host "Csak technikai proba. Az el nem fogadott grafika nem valik automatikusan veglegesse." -ForegroundColor Yellow
}
function Adb-Executable {
    $sdk=Need-Sdk
    $adb=Join-Path $sdk "platform-tools\adb.exe"
    if(-not (Test-Path $adb)){Fail "A platform-tools/adb.exe hianyzik az Android SDK-bol."}
    return $adb
}
function Pick-Device([string]$Adb){
    Run $Adb @("start-server")
    $lines=@(& $Adb "devices")
    $available=@()
    foreach($line in $lines){
        if($line -match '^(\S+)\s+device\s*$'){$available+= $Matches[1]}
    }
    if($DeviceSerial){
        if($available -notcontains $DeviceSerial){Fail "Ez a keszulek nincs engedelyezve: $DeviceSerial"}
        return $DeviceSerial
    }
    if($available.Count -eq 0){Fail "Nincs engedelyezett Android eszkoz. Kapcsold be az USB hibakeresest es hagyd jova a telefonon a PC-t."}
    if($available.Count -gt 1){
        Write-Host ("Tobb keszulek van: "+($available -join ", ")) -ForegroundColor Yellow
        Fail "Valassz egyet: PC_TAKS_WORLD.cmd -Mode install -DeviceSerial ESZKOZ_AZONOSITO"
    }
    return $available[0]
}
function Install-Android {
    if(-not (Test-Path $TestApk)){Build-Android}
    $adb=Adb-Executable
    $serial=Pick-Device $adb
    Say "Telepites $serial Androidra. Mentest NEM torlunk, alkalmazast NEM tavolitunk el."
    & $adb "-s" $serial "install" "-r" $TestApk
    if($LASTEXITCODE -ne 0){
        Fail "Telepitesi hiba. Alairas-utkozes eseten NEM toroljuk az elozo jatekot: kulon tesztcsomagot vagy azonos alairokulcsot kell hasznalni."
    }
    Say "Sikeres telepites. Inditsd a TAKS World TESZT alkalmazast a telefonon."
}
function Copy-Images {
    $file=$SourceZip
    if(-not $file){$file=Read-Host "A SuperImage.zip TELJES eleresi utja a PC-n"}
    if(-not (Test-Path -LiteralPath $file -PathType Leaf)){Fail "Nem talalom a ZIP-et: $file"}
    $adb=Adb-Executable
    $serial=Pick-Device $adb
    $remote="/sdcard/Download/SuperImage_TAKS_IMPORT.zip"
    & $adb "-s" $serial "shell" "ls" $remote *> $null
    if($LASTEXITCODE -eq 0){
        Say "A telefon Letoltesek mappajaban mar van SuperImage_TAKS_IMPORT.zip. Az ottani fajlt NEM irom felul."
        return
    }
    Run $adb @("-s",$serial,"push",$file,$remote)
    Say "Masolat a telefonon: Letoltesek / SuperImage_TAKS_IMPORT.zip"
    Say "A jatek Beallitasok menujeben valaszd ki az eredeti SuperImage ZIP-et."
}
function Tests {
    $python=Need-PythonVenv
    Push-Location $Root
    try{Run $python @("-m","unittest","discover","-s","tests","-p","test_superimage_ground_integration.py","-v")}
    finally{Pop-Location}
    Say "Forrasszintu ellenorzes OK. Ez NEM valos Android GPU/felbontas/FPS teszt."
}
function Open-Studio {
    $paths=@(
        "C:\Program Files\Android\Android Studio\bin\studio64.exe",
        "C:\Program Files\Android\Android Studio\bin\studio.exe"
    )
    foreach($studio in $paths){
        if(Test-Path $studio){Start-Process -FilePath $studio -ArgumentList @($Root);return}
    }
    Start-Process explorer.exe -ArgumentList @($Root)
    Say "Android Studio -> Open -> valaszd ki a TAKS World gyokermappajat: $Root"
}
function Capture-Screenshot {
    $adb=Adb-Executable
    $serial=Pick-Device $adb
    $stamp=Get-Date -Format "yyyyMMdd_HHmmss"
    $remote="/sdcard/Pictures/TAKS_World/TAKS_WORLD_$stamp.png"
    $local=Join-Path $Out "TAKS_WORLD_Telefon_Kep_$stamp.png"
    New-Item -ItemType Directory -Force $Out | Out-Null
    Run $adb @("-s",$serial,"shell","mkdir","-p","/sdcard/Pictures/TAKS_World")
    Run $adb @("-s",$serial,"shell","screencap","-p",$remote)
    Run $adb @("-s",$serial,"pull",$remote,$local)
    Say "Valodi Android-kep mentve: $local"
    Say "A telefonon is megmaradt: Kepek / TAKS_World."
}
function Capture-Logs {
    $adb=Adb-Executable
    $serial=Pick-Device $adb
    New-Item -ItemType Directory -Force $Out | Out-Null
    $logFile=Join-Path $Out "TAKS_WORLD_Android_hibanaplo.txt"
    & $adb "-s" $serial "logcat" "-d" "-v" "time" "-s" "AndroidRuntime:E" "TAKS_MENU_TOUCH:I" "TAKS_MODEL:E" "TAKS_SURFACE:E" "TAKS_RENDER:E" "TAKS_HUMAN:I" | Out-File -LiteralPath $logFile -Encoding utf8
    Say "Hibanaplo elmentve: $logFile"
}
function Check-PC {
    Say "TAKS WORLD PC ALLAPOT"
    Write-Host ("Projekt: "+$Root)
    Write-Host ("Android SDK: "+$(Sdk-Location))
    Write-Host ("Android Studio Java: "+$(if(Test-Path "C:\Program Files\Android\Android Studio\jbr\bin\java.exe"){"megtalalva"}else{"ellenorizendo"}))
    $gradle=Join-Path $Tools "gradle-8.9\bin\gradle.bat"
    Write-Host ("Helyi Gradle 8.9: "+$(if(Test-Path $gradle){"megtalalva"}else{"elso buildnel letoltodik"}))
    Write-Host ("Python Launcher: "+$(if(Find-Command "py.exe"){"megtalalva"}else{"ellenorizendo"}))
    Write-Host ("Elfogadott eredeti menu: "+$(if(Test-Path $ApprovedPath){"helyben"}else{"elso buildnel ellenorzott forrasbol"}))
    Write-Host ("Talajok a teszt APK-hoz: "+$(if(Test-Path (Join-Path $Root "app\src\main\assets\approved_ground\approved_ground_manifest.json")){"ellenorizheto"}else{"forras SuperImage.zip kell"}))
    Write-Host ("Teszt APK: "+$(if(Test-Path $TestApk){$TestApk}else{"meg nincs leforditva"}))
    Write-Host "Kovetelmenyek: Android SDK API35 + NDK27.2.12479018 + CMake3.22.1 + Java17+ + Python3.11+."
}
function Execute([string]$Action){
    Push-Location $Root
    try{
        switch($Action){
            "check"       { Check-PC }
            "build"       { Build-Android }
            "install"     { Install-Android }
            "tests"       { Tests }
            "studio"      { Open-Studio }
            "copy-images" { Copy-Images }
            "ground"      { Prepare-ApprovedGround }
            "screenshot"  { Capture-Screenshot }
            "logs"        { Capture-Logs }
            default       { Fail "Ismeretlen muvelet: $Action" }
        }
    }finally{Pop-Location}
}
if($Mode -ne "menu"){
    try{Execute $Mode;exit 0}catch{Write-Host ("HIBA: "+$_.Exception.Message) -ForegroundColor Red;exit 1}
}
while($true){
    Write-Host ""
    Write-Host "================ TAKS WORLD / PC -> ANDROID ================" -ForegroundColor Green
    Write-Host "1 - Rendszer ellenorzese"
    Write-Host "2 - Android TESZT APK epitese"
    Write-Host "3 - TESZT APK telepitese USB-n (nem torli a mentest)"
    Write-Host "4 - SuperImage.zip masolasa telefonra (az eredeti megmarad)"
    Write-Host "5 - Projekt megnyitasa Android Studioban"
    Write-Host "6 - Forraskod-ellenorzo tesztek"
    Write-Host "7 - Android hibanaplo mentese"
    Write-Host "8 - Eredeti SuperImage talajok elokeszitese az APK-ba"
    Write-Host "9 - Valodi Android jatek-kepkepernyo foto mentese PC-re"
    Write-Host "0 - Kilepes"
    $choice=Read-Host "Valassz"
    if($choice -eq "0"){exit 0}
    $actions=@{"1"="check";"2"="build";"3"="install";"4"="copy-images";"5"="studio";"6"="tests";"7"="logs";"8"="ground";"9"="screenshot"}
    $action=$actions[$choice]
    if(-not $action){Write-Host "Ismeretlen menupont." -ForegroundColor Yellow;continue}
    try{Execute $action}catch{Write-Host ("HIBA: "+$_.Exception.Message) -ForegroundColor Red}
    [void](Read-Host "ENTER = vissza a menube")
}
