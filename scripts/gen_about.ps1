# Generate hg_about_text.h from README.md.
# Run from the repository root: powershell -NoProfile -ExecutionPolicy Bypass -File scripts\gen_about.ps1
#
# The About window (F1) is a plain text box, so the README is turned into text a
# person can read there rather than copied in as Markdown. scripts\gen_about.py
# is the same program for hosts without PowerShell: the rules are listed at its
# top, and the two must produce the same bytes - change one, change the other.
$readmePath = 'README.md'
$outputPath = 'src/hg_about_text.h'
$enc = New-Object System.Text.UTF8Encoding($false)
if (-not (Test-Path $readmePath)) {
    $content = "#ifndef HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_README_W L`"(README.md not found)`"`r`n#endif"
    [System.IO.File]::WriteAllText($outputPath, $content, $enc)
    Write-Host "[Warning] README.md not found." -ForegroundColor Yellow
    exit 0
}

$NUL = [string][char]0
$DASH = [string][char]0x2014
$BULLET = [string][char]0x2022
$reCode = New-Object regex '`([^`]*)`'
$reLink = New-Object regex '\[([^\]]*)\]\(([^)]*)\)'
$reBold = New-Object regex '\*\*(.+?)\*\*'
$reItalic = New-Object regex '(?<![\w*])\*([^*\s](?:[^*]*[^*\s])?)\*(?![\w*])'
$reEscape = New-Object regex '\\([|*_`])'
$reStash = New-Object regex ($NUL + '(\d+)' + $NUL)
$reHeading = New-Object regex '^(#{1,6})\s+(.*)$'
$reItem = New-Object regex '^([-*+]|\d+\.)\s+(.*)$'
$reRule = New-Object regex '^(-{3,}|\*{3,}|_{3,})$'
$reCellSplit = New-Object regex '(?<!\\)\|'
$reSeparator = New-Object regex '^:?-+:?$'

$script:codes = New-Object System.Collections.Generic.List[string]
function Inline([string]$text) {
    $script:codes.Clear()
    $text = $reCode.Replace($text, [System.Text.RegularExpressions.MatchEvaluator]{ param($m)
        $script:codes.Add($m.Groups[1].Value); $NUL + ($script:codes.Count - 1) + $NUL })
    $text = $reLink.Replace($text, [System.Text.RegularExpressions.MatchEvaluator]{ param($m)
        $label = $m.Groups[1].Value; $url = $m.Groups[2].Value
        if ($url.StartsWith('http', [System.StringComparison]::Ordinal) -and $url -cne $label) { $label + ' (' + $url + ')' } else { $label } })
    $text = $reBold.Replace($text, '$1')
    $text = $reItalic.Replace($text, '$1')
    $text = $reEscape.Replace($text, '$1')
    $text = $reStash.Replace($text, [System.Text.RegularExpressions.MatchEvaluator]{ param($m)
        $script:codes[[int]$m.Groups[1].Value] })
    return $text
}

[byte[]]$bytes = [System.IO.File]::ReadAllBytes($readmePath)
$readme = $enc.GetString($bytes)

$out = New-Object System.Collections.Generic.List[string]   # one entry per line of the window; '' is a blank line
$marked = New-Object System.Collections.Generic.List[bool]  # per entry: still Markdown, converted once the paragraph is whole
$skip = $false
$fence = $false
$header = $null      # the header cells of the table being read, or $null
$para = $false       # the last entry is a paragraph or list item that a plain line continues
$first = $true
function Add([string]$text, [bool]$stillMarkdown = $false) { $out.Add($text); $marked.Add($stillMarkdown) }
function Blank { if ($out.Count -gt 0 -and $out[$out.Count - 1] -cne '') { Add '' } }

foreach ($raw in $readme.Split("`n")) {
    $line = $raw.TrimEnd("`r")
    $wasFirst = $first; $first = $false
    if ($line.Contains('<!-- SKIP_START -->')) { $skip = $true; continue }
    if ($line.Contains('<!-- SKIP_END -->')) { $skip = $false; continue }
    if ($skip -or $line.Contains('<!-- SKIP -->') -or $line.Trim().StartsWith('![', [System.StringComparison]::Ordinal)) { continue }
    if ($wasFirst -and -not $line.StartsWith('#', [System.StringComparison]::Ordinal)) { continue }   # the language / version / download line
    if ($line.StartsWith('**[Download', [System.StringComparison]::Ordinal)) { continue }             # a link to the file the reader is running

    # A quoted line is read as what it quotes: the > and the space after it go.
    $body = $line
    $quoted = $false
    while ($body.TrimStart(' ').StartsWith('>', [System.StringComparison]::Ordinal)) {
        $body = $body.TrimStart(' ').Substring(1)
        if ($body.StartsWith(' ', [System.StringComparison]::Ordinal)) { $body = $body.Substring(1) }
        $quoted = $true
    }

    if ($body.Trim().StartsWith('```', [System.StringComparison]::Ordinal)) { $fence = -not $fence; $para = $false; $header = $null; continue }
    if ($fence) { Add ('    ' + $body); continue }

    $s = $body.Trim()
    $indent = 0
    if (-not $quoted) { $indent = $body.Length - $body.TrimStart(' ').Length }

    if ($s -ceq '' -or $reRule.IsMatch($s)) { Blank; $para = $false; $header = $null; continue }

    $m = $reHeading.Match($s)
    if ($m.Success) {
        $level = $m.Groups[1].Value.Length; $title = Inline $m.Groups[2].Value
        Blank
        if ($level -eq 1) { Add $title } elseif ($level -eq 2) { Add ('== ' + $title + ' ==') } else { Add ('-- ' + $title + ' --') }
        Add ''
        $para = $false; $header = $null
        continue
    }

    if ($s.StartsWith('|', [System.StringComparison]::Ordinal)) {
        $split = $reCellSplit.Split($s)
        $cells = New-Object System.Collections.Generic.List[string]
        foreach ($c in $split) { $cells.Add($c.Trim()) }
        if ($cells.Count -ge 2) { $cells.RemoveAt($cells.Count - 1); $cells.RemoveAt(0) }
        $para = $false
        $allSeparator = ($cells.Count -gt 0)
        foreach ($c in $cells) { if (-not $reSeparator.IsMatch($c)) { $allSeparator = $false } }
        if ($allSeparator) { continue }
        for ($j = 0; $j -lt $cells.Count; $j++) { $cells[$j] = Inline $cells[$j] }
        if ($null -eq $header) { $header = $cells; continue }
        if ($cells.Count -eq 0) { continue }
        if ($cells.Count -le 2) {
            $text = '  ' + $cells[0]
            if ($cells.Count -eq 2 -and $cells[1] -cne '' -and $cells[1] -cne $DASH) { $text += ' ' + $DASH + ' ' + $cells[1] }
            Add $text
        } else {
            Add ('  ' + $cells[0])
            for ($j = 1; $j -lt $cells.Count; $j++) {
                if ($cells[$j] -ceq '' -or $cells[$j] -ceq $DASH) { continue }
                $name = ''
                if ($j -lt $header.Count) { $name = $header[$j] }
                if ($name -cne '') { Add ('      ' + $name + ': ' + $cells[$j]) } else { Add ('      ' + $cells[$j]) }
            }
        }
        continue
    }
    $header = $null

    $m = $reItem.Match($s)
    if ($m.Success) {
        $marker = $m.Groups[1].Value
        if ($marker -ceq '-' -or $marker -ceq '*' -or $marker -ceq '+') { $marker = $BULLET }
        Add ((' ' * $indent) + $marker + ' ' + $m.Groups[2].Value) $true
        $para = $true
        continue
    }

    # A paragraph or a list item is converted once all its lines are joined:
    # bold and links run across the README's hard line breaks.
    if ($para -and $out.Count -gt 0 -and $out[$out.Count - 1] -cne '') { $out[$out.Count - 1] = $out[$out.Count - 1] + ' ' + $s }
    else { Add $s $true; $para = $true }
}

while ($out.Count -gt 0 -and $out[$out.Count - 1] -ceq '') { $out.RemoveAt($out.Count - 1); $marked.RemoveAt($marked.Count - 1) }

$parts = New-Object System.Collections.Generic.List[string]
for ($i = 0; $i -lt $out.Count; $i++) {
    $text = $out[$i]
    if ($marked[$i]) { $text = Inline $text }
    $escaped = $text.Replace('\', '\\').Replace('"', '\"')
    $parts.Add('L"' + $escaped + '\r\n"')
}
$joined = [string]::Join(' ', $parts)
$content = "#ifndef HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_README_W $joined`r`n#endif"
[System.IO.File]::WriteAllText($outputPath, $content, $enc)
Write-Host "[Success] README.md processed successfully." -ForegroundColor Green
