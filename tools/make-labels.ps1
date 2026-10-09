Add-Type -AssemblyName System.Drawing
$ErrorActionPreference='Stop'
$rootPath = Split-Path $PSScriptRoot -Parent
$labels = [ordered]@{
settingsintro='اضبط رفّ على ذوقك';
appearancegroup='المظهر واللغة';
downloadgroup='خيارات التحميل';
pathsgroup='مسارات الألعاب';
updatesgroup='عن رفّ والتحديثات';
settingsgroupshint='L2 / R2 للتنقل بين المجموعات';
currentfolder='المسار الحالي';
folderhint='اضغط إكس لتغيير المسار';
languagehint='بدّل بين العربية والإنجليزية في أي وقت';
motionhint='حركة أقل مع تنقل سريع وواضح';
installhint='تُحفظ ملفاتك حتى تأكيد اكتمال التثبيت';
downloadhint='الاتصالات تتكيف مع خادم المصدر';
localdatahint='تُحفظ الإعدادات على جهازك';
updatenorepo='سيُربط مستودع التحديث الرسمي قبل النشر';
updateavailable='إصدار جديد من رفّ متاح';
updatedownloading='جاري تحميل التحديث';
updateverifying='جاري التحقق من سلامة الملفات';
updateready='التحديث جاهز للتثبيت';
updatecurrent='أنت تستخدم أحدث إصدار';
updateerror='تعذر إكمال التحديث';
updatecheck='البحث عن تحديث';
updateinstall='تثبيت وإغلاق رفّ';
updatedownload='تحميل التحديث';
updatepreserve='ألعابك وإعداداتك وتحميلاتك محفوظة';
updateconfirm='تثبيت التحديث وإغلاق رفّ؟';
updateclosehint='أوقف التحميلات مؤقتًا، ثم أعد فتح رفّ بعد اكتمال التثبيت';
updatenoticehint='Options ← عن رفّ والتحديثات';
 viewdownloads='اذهب للتحميلات';
 reconnecting='جاري إعادة الاتصال تلقائيًا'; torrentchecking='جاري فحص ملفات التورنت';
 library='المكتبة'; ps2='بلايستيشن ٢'; switch='نينتندو سويتش'; downloads='التحميلات'; favorites='المفضلة'; settings='الإعدادات'; search='ابحث عن لعبتك'; featured='المختارة'; rating='الأعلى تقييماً'; popular='الأكثر تحميلاً'; az='بالاسم'; smallest='الأقل حجماً'; all='الكل'; ready='جاهز'; connecting='جاري الاتصال'; offline='الخدمة غير متاحة'; options='اختر المصدر والنسخة'; download='أضف إلى التحميلات'; source='المصدر'; version='النسخة'; queue='قائمة التحميل'; queued='في الانتظار'; downloading='جاري التحميل'; paused='متوقف مؤقتاً'; extracting='جاري فك الضغط'; installing='جاري نقل الملفات'; done='اكتمل'; error='تعذر إكمال العملية'; canceled='تم الإلغاء'; remaining='الوقت المتبقي'; calculating='جاري الحساب'; speed='السرعة'; resume='استكمال'; pause='إيقاف مؤقت'; cancel='إلغاء التحميل'; back='رجوع'; select='اختيار'; sort='ترتيب'; language='اللغة'; parallel='ألعاب في وقت واحد'; connections='اتصالات لكل ملف'; saved='تم الحفظ'; added='أضيفت إلى التحميلات'; exists='اللعبة مثبتة أو موجودة بالقائمة'; empty='لا توجد نتائج'; nojobs='لم تبدأ أي تحميل بعد'; hint='اكتشف لعبة جديدة'; favorite='أضف للمفضلة'; removefavorite='إزالة من المفضلة'; exit='إغلاق رفّ؟'; yes='تأكيد'; no='رجوع'; confirmcancel='إلغاء هذه المهمة؟'; clear='مسح'; space='مسافة'; apply='بحث'; catalog='لعبة في المكتبة'; mirrors='مصادر ونسخ'; unknown='غير متوفر'; installed='مثبتة'; seconds='ثانية'; minutes='دقيقة'; hours='ساعة'; ps2path='مسار ألعاب بلايستيشن ٢'; switchpath='مسار ألعاب سويتش'; restart='أعد فتح التطبيق'; serviceerror='شغّل الجلبريك ثم أعد فتح رفّ'; refresh='تحديث'; local='على الجهاز'; actualratings='التقييمات من ميتاكريتيك'; actualdownloads='التحميلات حسب إحصاءات المصدر'; transfers='تستمر التحميلات عند إغلاق رفّ'; arabic='العربية'; english='الإنجليزية'; noscore='الألعاب التي تملك بيانات فقط'; wait='انتظر لحظة'; more='خيارات'; collection='مجموعة'; footer='اختيار        بحث        ترتيب        اللغة        رجوع'; detailfooter='تحميل        المفضلة        رجوع'; jobsfooter='إيقاف أو استكمال        إلغاء المهمة        رجوع'
}
$labels.ps5='بلايستيشن ٥';$labels.yourlibrary='عالم ألعابك';$labels.pauseresume='إيقاف أو استكمال';$labels.change='تغيير';$labels.searchshort='بحث';$labels.tabs='الأقسام';$labels.backspace='حذف';$labels.ps5untested='تشغيل ألعاب بلايستيشن ٥ على جهازك لم يُختبر بعد';$labels.ps5path='ملفات بلايستيشن ٤ و٥'
$labels.storetitle='اكتشف لعبتك القادمة';$labels.noactive='لا توجد تحميلات حالية';$labels.completedwhere='الألعاب المكتملة تجدها في قسم مكتبتي';$labels.ps4='بلايستيشن ٤';$labels.ps4path='مسار ألعاب بلايستيشن ٤'
$labels.consoleuntested='تنزيل ملفات فقط — التثبيت والتشغيل حسب دعم جهازك';$labels.downloaded='محفوظة'
$labels.gamepath='مسار اللعبة';$labels.savepath='حفظ';$labels.scanpaths='البحث التلقائي عن المسارات';$labels.scanning='جاري الفحص';$labels.scanstart='افحص الآن';$labels.lettercase='حجم الحروف';$labels.ps5path='مسار ألعاب بلايستيشن ٥'
$labels.defaultpath='المسار الافتراضي'
$labels.Remove('footer');$labels.Remove('detailfooter');$labels.Remove('jobsfooter')
$labels.systempages='الصفحات';$labels.systemshome='عوالمك، في مكتبة واحدة';$labels.systemslabel='جهاز';$labels.allsystems='كل الأجهزة';$labels.makerps='بلايستيشن';$labels.makernintendo='نينتندو';$labels.makerxbox='إكس بوكس';$labels.makersega='سيجا';$labels.makerarcade='أركيد';$labels.systemshint='اختر جهازك واستكشف ألعابه';$labels.opensystem='افتح المكتبة';$labels.manufacturer='الشركة'
$labels.rafftitle='رفّ — مكتبة ألعاب';$labels.developer='تطوير محمد الرويلي';$labels.developedby='من تطوير محمد الرويلي';$labels.allplatforms='كل المنصات';$labels.biosrequired='قد يلزم BIOS من جهازك داخل RetroArch/system؛ التوافق حسب اللعبة';$labels.splashfeature='ألعابك في مكان واحد';$labels.splashfeature2='تحميل متعدد • مسارات مخصصة • عربي وإنجليزي';$labels.loading='جاري تجهيز مكتبتك'
$labels.autoInstall='تثبيت PKG تلقائيًا';$labels.removePackages='حذف PKG بعد تأكيد التثبيت';$labels.pkginstall='جاري تثبيت اللعبة';$labels.pkgcleanup='جاري توفير المساحة';$labels.enabled='مفعّل';$labels.disabled='معطّل'
$labels.consoleuntested='تثبيت PKG تلقائيًا عند تفعيل الخيار؛ التشغيل حسب دعم جهازك'
$labels.actualratings='التقييمات حسب المصدر المتاح';$labels.actualdownloads='قوائم PlayStation وإحصاءات المصادر';$labels.noscore='الألعاب بلا بيانات تظهر في نهاية القائمة'
foreach($unused in 'ps5untested','arabic','english','collection','more','hint'){$labels.Remove($unused)}
$labels.sourceerror='المصدر غير متاح مؤقتًا';$labels.spaceerror='المساحة غير كافية';$labels.installerror='التثبيت غير مؤكد؛ ملف PKG محفوظ';$labels.networkerror='انقطع التحميل؛ اضغط إكس للاستكمال';$labels.fileerror='تعذرت عملية الملفات'
$labels.ps1='بلايستيشن ١';$labels.ps1path='مسار ألعاب بلايستيشن ١';$labels.metadata='فهرس ألعاب Libretro';$labels.metadataonly='لا يوجد مصدر تنزيل مرتبط بهذه اللعبة';$labels.ps1import='ضع ملف لعبتك في المسار الظاهر أدناه';$labels.ps1open='افتح محاكي بلايستيشن ١ واختر اللعبة'
foreach($unused in 'seconds','minutes','hours','restart','refresh','serviceerror'){$labels.Remove($unused)}
$labels.available='متاحة للتنزيل';$labels.filtertitle='الترتيب والفلترة';$labels.showgames='عرض الألعاب';$labels.applyfilter='تطبيق الاختيار';$labels.filterhint='غيّر الفلتر أو جرّب البحث باسم آخر';$labels.browsefast='تنقل بين الصفحات';$labels.gamedetails='تفاصيل اللعبة';$labels.sourcechecked='تم فحص المصدر؛ قد يتغير توفره';$labels.metadata='بيانات اللعبة';$labels.catalog='لعبة';$labels.mirrors='مصادر ونسخ';$labels.select='التفاصيل';$labels.sort='ترتيب وفلترة'
$labels.xbox='إكس بوكس';$labels.xbox360='إكس بوكس ٣٦٠';$labels.xboxpath='مسار ألعاب إكس بوكس';$labels.xbox360path='مسار ألعاب إكس بوكس ٣٦٠'
$labels.allgames='الألعاب';$labels.emulator='المحاكي والمنصة';$labels.reducedmotion='تقليل الحركة';$labels.searchresults='نتائج البحث'
$labels.connections='الحد الأعلى للاتصالات الذكية'
$labels.browsegrid='تصفّح';$labels.discover='اكتشف';$labels.library='مكتبتي';$labels.emulator='الأجهزة والمحاكيات'
foreach($unused in 'footer','detailfooter','jobsfooter','nojobs','calculating'){$labels.Remove($unused)}
$labels.Remove('connecting');$labels.Remove('noscore')
$labels.ps3='بلايستيشن ٣';$labels.ps3path='مسار ألعاب بلايستيشن ٣';$labels.ps3notice='RPCS3 تجريبي؛ ثبّت PKG داخله مع ترخيص لعبتك'
$labels['hub']='المحاكيات والألعاب الحرة'
$labels['emulators']='المحاكيات'
$labels['freegames']='ألعاب حرة'
$labels['sources']='المصادر'
$labels['sourceindex']='دليل خارجي؛ يختلف الإذن والتوافق لكل ملف'
$labels['importtip']='استورد ملف تورنت أو رابط مغناطيسي مصرحاً لك باستخدامه'
$labels['nobuild']='مرجع للمشروع؛ لا توجد حزمة جاهزة متحقق منها'
$labels['downloadonly']='تنزيل فقط؛ لم يُتحقق من تثبيت وتشغيل هذه الحزمة'
$labels['transfercenter']='مركز نقل الملفات'
$labels['torrentnotice']='التورنت يرفع أجزاء من الملف للآخرين أثناء التحميل'
$labels['torrentnotice2']='عنوان اتصالك ظاهر للأقران؛ استخدم الملفات المصرح بها فقط'
$labels['seedzero']='صفر دقيقة يوقف المشاركة بعد الاكتمال؛ الرفع أثناء التنزيل مستمر'
$labels['agree']='أوافق وأتابع'
$labels['importfolder']='ضع ملفات torrent أو magnet في /data/raff/imports'
$labels['magnet']='إدخال رابط مغناطيسي'
$labels['destination']='مسار الألعاب'
$labels['completedfiles']='الملفات المكتملة'
$labels['selectfiles']='اختيار الملفات'
$labels['startdownload']='تحميل المحدد'
$labels['pages']='صفحات الملفات'
$labels['downloadlimit']='حد التنزيل KiB/s؛ صفر بلا حد'
$labels['uploadlimit']='حد الرفع KiB/s'
$labels['seedtime']='المشاركة بعد الاكتمال بالدقائق'
$labels['clearhistory']='إخفاء السجل المكتمل مع إبقاء الملفات'
$labels['torrentsettings']='التورنت: السرعة والمشاركة'
$labels['importtorrent']='استيراد تورنت'
$labels['categories']='القسم'
$labels['hostfilter']='فلتر الجهاز'
$labels['refresh']='تحديث'
$labels['metadata']='جلب معلومات التورنت'
$labels['selecting']='اختر ملفات التورنت أولاً'
$labels['seeding']='اكتمل التنزيل؛ مشاركة الملفات'
$labels['stalled']='لا يوجد تقدم؛ استأنف للمحاولة'
$labels['retrying']='إعادة المحاولة'
$labels['metadata-queued']='بانتظار جلب معلومات التورنت'
$labels['noresults']='لا توجد نتائج'
$labels['metadataonly']='بيانات اللعبة؛ لا يوجد تنزيل متحقق من إذنه'
$labels['consoleuntested']='تنزيل الملف لا يعني إمكانية تثبيته أو تشغيله على جهازك'
$labels['gamequeued']='أضيفت اللعبة إلى التحميلات'
$labels['gameselected']='سيتم تحميل ملفات هذه النسخة فقط'
$labels['aboutgame']='معلومات اللعبة'
$labels['metadataonly']='لا يوجد مصدر تنزيل مرتبط بهذه اللعبة'
if($labels.Count -gt 256){throw 'Arabic atlas capacity exceeded'}
$bitmap = [Drawing.Bitmap]::new(2048,8192,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics=[Drawing.Graphics]::FromImage($bitmap)
$graphics.Clear([Drawing.Color]::Transparent)
$graphics.TextRenderingHint=[Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$font=[Drawing.Font]::new('Segoe UI',38,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
$format=New-Object Drawing.StringFormat
$format.FormatFlags=[Drawing.StringFormatFlags]::DirectionRightToLeft
$format.Alignment=[Drawing.StringAlignment]::Near
$map=[ordered]@{};$y=0;$x=0;$index=0
foreach($key in $labels.Keys){
 $x=[Math]::Floor($index/128)*1024;$y=($index%128)*64;$index++;$text=$labels[$key];$size=$graphics.MeasureString($text,$font,1000,$format)
 $width=[int][Math]::Ceiling($size.Width+10)
 $rectangle=New-Object Drawing.RectangleF $x,$y,$width,64
 $graphics.DrawString($text,$font,[Drawing.Brushes]::White,$rectangle,$format)
 $map[$key]=@{x=$x;y=$y;w=$width;h=64;text=$text};$y+=64
}
$extraHeight=[Math]::Max(64,($labels.Count-128)*64)
$primaryRect=[Drawing.Rectangle]::new(0,0,1024,8192)
$extraRect=[Drawing.Rectangle]::new(1024,0,1024,$extraHeight)
$primary=$bitmap.Clone($primaryRect,$bitmap.PixelFormat)
$extra=$bitmap.Clone($extraRect,$bitmap.PixelFormat)
$primary.Save((Join-Path $rootPath 'assets/arabic.png'),[Drawing.Imaging.ImageFormat]::Png)
$extra.Save((Join-Path $rootPath 'assets/arabic-extra.png'),[Drawing.Imaging.ImageFormat]::Png)
$primary.Dispose();$extra.Dispose()
foreach($key in $map.Keys){$entry=$map[$key];$entry.atlas=[int]($entry.x -ge 1024);$entry.atlasH=8192;if($entry.atlas -eq 1){$entry.atlasH=$extraHeight};$entry.x=0}
[IO.File]::WriteAllText((Join-Path $rootPath 'assets/arabic.json'),($map|ConvertTo-Json -Depth 3),[Text.UTF8Encoding]::new($false))
$graphics.Dispose();$bitmap.Dispose();$font.Dispose();$format.Dispose()
Write-Output "Baked $($labels.Count) Arabic labels"
