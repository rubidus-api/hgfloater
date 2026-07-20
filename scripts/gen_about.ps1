# Generate hg_about_text.h from README.md.
# Run from the repository root: powershell -NoProfile -ExecutionPolicy Bypass -File scripts\gen_about.ps1
$readmePath = 'README.md'
$outputPath = 'src/hg_about_text.h'
if (Test-Path $readmePath) {
    [byte[]]$bytes = [System.IO.File]::ReadAllBytes($readmePath)
    $enc = New-Object System.Text.UTF8Encoding($false)
    $readme = $enc.GetString($bytes)
    $lines = $readme -split '\r?\n'
    $skip = $false
    $output = @()
    foreach ($line in $lines) {
        if ($line.Contains('<!-- SKIP_START -->')) { $skip = $true; continue }
        if ($line.Contains('<!-- SKIP_END -->')) { $skip = $false; continue }
        if ($skip -or $line.Contains('<!-- SKIP -->') -or $line.Trim().StartsWith('![')) { continue }
        $escaped = $line.Replace('\', '\\').Replace('"', '\"')
        $output += "L`"$escaped\r\n`""
    }
    $joined = $output -join ' '
    $content = "#ifndef HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_README_W $joined`r`n#endif"
    [System.IO.File]::WriteAllText($outputPath, $content, $enc)
    Write-Host "[Success] README.md processed successfully." -ForegroundColor Green
} else {
    $content = "#ifndef HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_README_W L`"(README.md not found)`"`r`n#endif"
    Set-Content -Path $outputPath -Value $content -Encoding UTF8
    Write-Host "[Warning] README.md not found." -ForegroundColor Yellow
}
