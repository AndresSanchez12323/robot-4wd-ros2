$ErrorActionPreference = 'Stop'
$resultado = Join-Path $PSScriptRoot 'resultado.json'
$registro = Join-Path $PSScriptRoot 'desinstalacion.log'
$paquetes = @(
    @{ Inf = 'oem191.inf'; Hash = '9B4DDF8F4A513D3D32D7A6A6922D445CFF17EA41D952E591F93B74D17B94D18A' },
    @{ Inf = 'oem16.inf'; Hash = 'FD3478754F387FEA18AB22D72220F0A73224D7D6682BC0B41011B5227AB32B57' }
)
$eliminados = @()
$reinicio = $false
try {
    Start-Transcript -Path $registro -Append | Out-Null
    $principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw 'Se necesita ejecutar como administrador.' }
    # Verificar identidad de TODOS los paquetes antes de modificar el almacen.
    foreach ($p in $paquetes) {
        $inf = Join-Path $env:windir ('INF\' + $p.Inf)
        if (!(Test-Path -LiteralPath $inf)) { continue }
        if ((Get-FileHash -LiteralPath $inf -Algorithm SHA256).Hash -ne $p.Hash) { throw ('Cambio la identidad de ' + $p.Inf) }
        $contenido = Get-Content -LiteralPath $inf -Raw
        if ($contenido -notmatch 'Silicon Laboratories' -or $contenido -notmatch 'CP210') { throw 'El paquete no es CP210x.' }
    }
    # Respaldar ambos paquetes antes de desinstalar cualquiera. Sin /force ni /reboot.
    foreach ($p in $paquetes) {
        if (!(Test-Path -LiteralPath (Join-Path $env:windir ('INF\' + $p.Inf)))) { continue }
        $carpeta = Join-Path $PSScriptRoot ('respaldo\' + $p.Inf)
        New-Item -ItemType Directory -Path $carpeta -Force | Out-Null
        & "$env:windir\System32\pnputil.exe" /export-driver $p.Inf $carpeta
        if ($LASTEXITCODE -ne 0) { throw ('No se pudo respaldar ' + $p.Inf) }
        if (!(Get-ChildItem -LiteralPath $carpeta -Recurse -Filter *.inf)) { throw 'Respaldo sin INF.' }
    }
    foreach ($p in $paquetes) {
        if (!(Test-Path -LiteralPath (Join-Path $env:windir ('INF\' + $p.Inf)))) { continue }
        & "$env:windir\System32\pnputil.exe" /delete-driver $p.Inf /uninstall
        $codigo = $LASTEXITCODE
        if ($codigo -ne 0 -and $codigo -ne 3010) { throw ('No se pudo desinstalar ' + $p.Inf + '; codigo ' + $codigo) }
        if ($codigo -eq 3010) { $reinicio = $true }
        $eliminados += $p.Inf
    }
    @{ Estado='Completado'; Eliminados=$eliminados; ReinicioNecesario=$reinicio } | ConvertTo-Json | Set-Content -LiteralPath $resultado -Encoding UTF8
} catch {
    @{ Estado='Error'; Error=$_.Exception.Message; Eliminados=$eliminados } | ConvertTo-Json | Set-Content -LiteralPath $resultado -Encoding UTF8
    Write-Output $_.Exception.Message
} finally {
    try { Stop-Transcript | Out-Null } catch {}
}
