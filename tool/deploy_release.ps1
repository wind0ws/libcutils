<#
.SYNOPSIS
  libcutils 一键发版编排器 (windows / linux / android / 任意 toolchain 交叉平台)。

.DESCRIPTION
  复用 tool/ 下既有子脚本:
    windows      -> deploy_for_windows.bat  (cmd, pthread_mode=1=posix, VS 自动探测)
    android      -> deploy_for_android.bat  (cmd, ninja+NDK)
    linux        -> deploy_for_linux.sh     (WSL bash, m64+m32)
    其他平台     -> make_cross_platform.sh  (WSL bash, 交叉编译, 依赖 cmake/toolchains/<平台>.toolchain.cmake)
  默认全部 Release。构建产物落到 tool/deploy/<type>/<platform>_<abi>/,
  头文件落到 tool/deploy/inc/。完成后可打包成 tar.gz 归档。

.PARAMETER Platforms
  要发版的平台子集,默认全部四个。可选: windows android linux linaro7.5.0

.PARAMETER BuildType
  构建类型,默认 Release。

.PARAMETER Distro
  linux/linaro 交叉编译使用的 WSL 发行版,默认 ubuntu_16.04。

.PARAMETER NoPackage
  跳过 tar.gz 打包,只产出 deploy 目录。

.EXAMPLE
  .\deploy_release.ps1
  .\deploy_release.ps1 -Platforms windows,linux
  .\deploy_release.ps1 -BuildType Release -Distro ubuntu_22.04
#>
[CmdletBinding()]
param(
    [string[]] $Platforms = @('windows','android','linux','linaro7.5.0'),
    [ValidateSet('Release','Debug','MinSizeRel','RelWithDebInfo')]
    [string]   $BuildType = 'Release',
    [string]   $Distro    = 'ubuntu_16.04',
    [switch]   $NoPackage
)

$ErrorActionPreference = 'Stop'
$ToolDir = $PSScriptRoot
Set-Location $ToolDir

function Write-Section([string]$msg) {
    Write-Host ""
    Write-Host ("=" * 70) -ForegroundColor Cyan
    Write-Host "  $msg" -ForegroundColor Cyan
    Write-Host ("=" * 70) -ForegroundColor Cyan
}

# 校验平台名: 内建 windows/android/linux 直通; 其余查 toolchain 文件存在性
function Test-Platform([string]$platform) {
    if ($platform -in @('windows','android','linux')) { return $true }
    $tc = Join-Path (Join-Path $ToolDir "cmake\toolchains") "$platform.toolchain.cmake"
    return (Test-Path $tc)
}

# 列出全部可用平台 (内建 + toolchains 目录)，用于报错提示
function Get-AvailablePlatforms {
    $builtin = @('windows','android','linux')
    $tcDir = Join-Path $ToolDir "cmake\toolchains"
    $tc = @()
    if (Test-Path $tcDir) {
        $tc = Get-ChildItem $tcDir -Filter "*.toolchain.cmake" |
              ForEach-Object { $_.Name -replace '\.toolchain\.cmake$','' }
    }
    return ($builtin + $tc) | Sort-Object -Unique
}

# 平台 -> 产物目录前缀 glob (windows 实际目录是 windows<年份>_，故用 windows*)
function Get-PlatformGlob([string]$platform) {
    if ($platform -eq 'windows') { return 'windows*_*' }
    return "${platform}_*"
}

# 校验 WSL 发行版存在 (wsl -l 输出为 UTF-16LE)
function Test-WslDistro([string]$name) {
    $raw = & wsl.exe -l -q 2>$null
    $names = ($raw -join "`n") -replace "`0","" -split "`r?`n" | ForEach-Object { $_.Trim() } | Where-Object { $_ }
    return ($names -contains $name)
}

# 在 cmd 域调用 .bat 子脚本。显式切到 tool/ (子进程 CWD 不随 PS Set-Location 改变);
# 清空 NoDefaultCurrentDirectoryInExePath (某些 shell/CI 会注入=1,导致既有脚本间的
# 裸名 call make_windows.bat 等无法从当前目录解析);用 call .\<bat> 进一步加固。
# 子进程 stdout/stderr 直通控制台,仅以 [int] 退出码作为返回值。
function Invoke-CmdBat([string]$batName, [string]$batArgs) {
    Write-Host "[cmd] $batName $batArgs" -ForegroundColor DarkGray
    & cmd.exe /c "set `"NoDefaultCurrentDirectoryInExePath=`" && cd /d `"$ToolDir`" && call .\$batName $batArgs" | Out-Host
    return [int]$LASTEXITCODE
}

# 在指定 WSL 发行版内执行 bash 脚本,工作目录切到 tool/。
function Invoke-WslBash([string]$distro, [string]$bashCmd) {
    Write-Host "[wsl:$distro] $bashCmd" -ForegroundColor DarkGray
    $toolUnix = (& wsl.exe -d $distro -e wslpath -a "$ToolDir") | Select-Object -First 1
    $toolUnix = "$toolUnix".Trim()
    & wsl.exe -d $distro -e bash -lc "cd '$toolUnix' && $bashCmd" | Out-Host
    return [int]$LASTEXITCODE
}

# 各平台构建动作。返回纯整数退出码,0 表示成功。
function Build-Platform([string]$platform) {
    switch ($platform) {
        'windows' {
            # pthread_mode=1 = pthreads-win32 静态库 (真正的 posix pthread 实现),已在 .bat 内硬编码
            return (Invoke-CmdBat "deploy_for_windows.bat" "$BuildType")
        }
        'android' {
            return (Invoke-CmdBat "deploy_for_android.bat" "$BuildType c++_static")
        }
        'linux' {
            return (Invoke-WslBash $Distro "dos2unix -q deploy_for_linux.sh make_cross_platform.sh setup_env.sh; chmod +x ./deploy_for_linux.sh; ./deploy_for_linux.sh $BuildType")
        }
        default {
            # 任意 toolchain 交叉平台 (linaro7.5.0 / hisi_a7 / r328 ...)
            return (Invoke-WslBash $Distro "dos2unix -q make_cross_platform.sh setup_env.sh; chmod +x ./make_cross_platform.sh; ./make_cross_platform.sh $platform $BuildType")
        }
    }
}

# ---- 平台校验 ----
Write-Section "libcutils 一键发版  type=$BuildType  platforms=$($Platforms -join ',')"

$invalid = @($Platforms | Where-Object { -not (Test-Platform $_) })
if ($invalid.Count -gt 0) {
    $avail = Get-AvailablePlatforms
    throw "无效平台: $($invalid -join ', ')。可用平台: $($avail -join ', ')"
}

# ---- 构建前清理对应平台旧产物目录 ----
$typeDir = $BuildType.ToLower()
$deployTypeDir = Join-Path (Join-Path $ToolDir 'deploy') $typeDir
if (Test-Path $deployTypeDir) {
    foreach ($p in $Platforms) {
        $glob = Get-PlatformGlob $p
        $old = @(Get-ChildItem $deployTypeDir -Directory -ErrorAction SilentlyContinue |
                 Where-Object { $_.Name -like $glob })
        if ($old.Count -gt 0) {
            Write-Host "清理旧产物 ($p): $($old.Name -join ', ')" -ForegroundColor DarkGray
            $old | Remove-Item -Recurse -Force
        }
    }
}

# ---- WSL 预检 ----
$needWsl = $Platforms | Where-Object { $_ -notin @('windows','android') }
if ($needWsl) {
    if (-not (Test-WslDistro $Distro)) {
        $dl = (& wsl.exe -l -q 2>$null) -replace "`0","" -split "`r?`n" | Where-Object { $_.Trim() }
        throw "WSL 发行版 '$Distro' 不存在。可用列表: $($dl -join ', ')"
    }
    Write-Host "WSL 发行版 '$Distro' 就绪 (用于: $($needWsl -join ', '))" -ForegroundColor Green
}

# ---- 逐平台构建 ----
$results = [ordered]@{}
$startAll = Get-Date
foreach ($p in $Platforms) {
    Write-Section "构建 $p ($BuildType)"
    $t0 = Get-Date
    $code = 1
    try {
        $code = Build-Platform $p
    } catch {
        Write-Host "异常: $_" -ForegroundColor Red
        $code = 99
    }
    $elapsed = [int]((Get-Date) - $t0).TotalSeconds
    $results[$p] = @{ Code = $code; Seconds = $elapsed }
    if ($code -eq 0) {
        Write-Host "OK  $p  (${elapsed}s)" -ForegroundColor Green
    } else {
        Write-Host "FAIL  $p  exit=$code  (${elapsed}s)" -ForegroundColor Red
    }
}

# ---- 汇总 ----
Write-Section "构建结果汇总"
$failed = @()
foreach ($p in $results.Keys) {
    $r = $results[$p]
    $tag = if ($r.Code -eq 0) { 'OK  ' } else { 'FAIL'; }
    $color = if ($r.Code -eq 0) { 'Green' } else { 'Red' }
    Write-Host ("  {0,-14} {1}  exit={2,-3} {3}s" -f $p, $tag, $r.Code, $r.Seconds) -ForegroundColor $color
    if ($r.Code -ne 0) { $failed += $p }
}
$totalSec = [int]((Get-Date) - $startAll).TotalSeconds
Write-Host ("  总耗时: {0}s" -f $totalSec) -ForegroundColor Cyan

if ($failed.Count -gt 0) {
    Write-Host ""
    Write-Host "存在失败平台: $($failed -join ', ') — 跳过打包。" -ForegroundColor Red
    exit 1
}

# ---- 打包 (tar.gz) ----
$deployDir = Join-Path $ToolDir 'deploy'
$typeDir   = $BuildType.ToLower()
if (-not $NoPackage) {
    Write-Section "打包归档 (tar.gz)"
    # 版本号: 优先 git describe,回退 CMake 工程版本
    $ver = (& git -C $ToolDir describe --tags 2>$null)
    if (-not $ver) { $ver = '1.8.0' }
    $ver = $ver.Trim()

    $archiveDir = Join-Path $deployDir '__archive__'
    if (-not (Test-Path $archiveDir)) { New-Item -ItemType Directory -Path $archiveDir | Out-Null }
    $stamp   = Get-Date -Format 'yyyyMMdd'
    $tarName = "lcu_${ver}_${typeDir}_${stamp}.tar.gz"
    $tarPath = Join-Path $archiveDir $tarName

    # 只打包本次构建的平台目录 + 公共 inc/
    $abiMap = @{ 'windows' = @('windows_x32','windows_x64'); 'linux' = @('linux_x32','linux_x64'); 'android' = @('android_armeabi-v7a','android_arm64-v8a','android_x86','android_x86_64'); 'linaro7.5.0' = @('linaro7.5.0_x64') }
    $relItems = @("inc")
    foreach ($p in $Platforms) {
        foreach ($abi in $abiMap[$p]) {
            $candidate = Join-Path $deployDir (Join-Path $typeDir $abi)
            if (Test-Path $candidate) { $relItems += "$typeDir/$abi" }
        }
    }
    Write-Host "归档内容: $($relItems -join ', ')" -ForegroundColor DarkGray
    # 切到 deploy/ 内执行 tar,归档输出也用相对路径,避免 Windows bsdtar 把 'E:' 误判为远程主机
    Push-Location $deployDir
    try {
        & tar.exe --force-local -czf "__archive__/$tarName" $relItems
        if ($LASTEXITCODE -ne 0) { throw "tar 打包失败 exit=$LASTEXITCODE" }
    } finally { Pop-Location }
    $sizeMB = [math]::Round((Get-Item $tarPath).Length / 1MB, 2)
    Write-Host "归档完成: $tarPath  (${sizeMB} MB)" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "已跳过打包 (-NoPackage)。产物位于: $deployDir\$typeDir\" -ForegroundColor Yellow
}

Write-Section "发版完成 ✅  ($BuildType)"
exit 0
