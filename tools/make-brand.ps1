$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$rootPath=Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory -Force "$rootPath/assets/brand" | Out-Null
$bmp=[Drawing.Bitmap]::new(512,512)
$g=[Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint=[Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$background=[Drawing.Drawing2D.LinearGradientBrush]::new([Drawing.Rectangle]::new(0,0,512,512),[Drawing.ColorTranslator]::FromHtml('#25202e'),[Drawing.ColorTranslator]::FromHtml('#090b13'),60.0)
$g.FillRectangle($background,0,0,512,512)
function Rounded($x,$y,$w,$h,$radius,$color){
 $path=[Drawing.Drawing2D.GraphicsPath]::new();$diameter=$radius*2
 $path.AddArc($x,$y,$diameter,$diameter,180,90);$path.AddArc($x+$w-$diameter,$y,$diameter,$diameter,270,90)
 $path.AddArc($x+$w-$diameter,$y+$h-$diameter,$diameter,$diameter,0,90);$path.AddArc($x,$y+$h-$diameter,$diameter,$diameter,90,90);$path.CloseFigure()
 $brush=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml($color));$g.FillPath($brush,$path);$brush.Dispose();$path.Dispose()
}
# Quiet, layered light; the three game cases form Raff's shelf mark.
for($n=30;$n -gt 0;$n--){$halo=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(2,255,67,97));$g.FillEllipse($halo,95-$n*3,25-$n*2,320+$n*6,240+$n*4);$halo.Dispose()}
Rounded 151 104 58 140 14 '#ff896a'
Rounded 227 71 58 173 14 '#ff4968'
Rounded 303 104 58 140 14 '#c72951'
Rounded 137 261 238 9 4 '#f7e8e9'
$shine=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(48,255,255,255));$g.FillRectangle($shine,237,87,3,102);$shine.Dispose()
$font=[Drawing.Font]::new('Segoe UI',116,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
$sub=[Drawing.Font]::new('Segoe UI',33,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
$fmt=[Drawing.StringFormat]::new();$fmt.Alignment=[Drawing.StringAlignment]::Center;$fmt.FormatFlags=[Drawing.StringFormatFlags]::DirectionRightToLeft
$g.DrawString('رفّ',$font,[Drawing.Brushes]::White,[Drawing.RectangleF]::new(0,274,512,147),$fmt)
$muted=[Drawing.SolidBrush]::new([Drawing.ColorTranslator]::FromHtml('#c9bacb'))
$g.DrawString('مكتبة ألعاب',$sub,$muted,[Drawing.RectangleF]::new(0,427,512,59),$fmt)
$bmp.Save("$rootPath/assets/brand/icon0.png",[Drawing.Imaging.ImageFormat]::Png)
$g.Dispose();$bmp.Dispose();$background.Dispose();$font.Dispose();$sub.Dispose();$fmt.Dispose();$muted.Dispose()
Write-Output 'Built Raff cinema app icon (512 x 512)'
