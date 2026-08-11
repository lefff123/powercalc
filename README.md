# PowerCalc

Расчёт установившихся режимов ЭС (Ньютон) + редактор отчётов с живым превью и экспортом HTML/PDF.

## Сборка (Linux)

Требуются: CMake ≥ 3.16, C++17, Qt6 (Widgets, WebEngine, Network), QuaZip (Qt6), yaml-cpp.
KaTeX-ассеты лежат в ресурсах (qrc).

    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build -j$(nproc)

## Windows: чёрный экран / краш превью

Перед запуском:

    set QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu
    set QT_OPENGL=software

Если всё равно крашится:

    set QTWEBENGINE_DISABLE_SANDBOX=1

## Синтаксис документа (v1.2)

**YAML-мета**: title/author/date, show_substitution, align, heading_align,
page.size (A4/A3/A5/Letter), page.margin (cm/mm), text.size (pt).

**Формулы**: `$$R = 10 &Ом$$` — присваивание + единица серым; `$$hide … $$` — скрытый блок;
`$$! Z = R+1$$` — без подстановки; `$$u=$$` — текущее значение;
`$$x = 1$$ {align:center}` — локальный стиль после закрывающего `$$`.

**Inline в тексте**: `$$\Delta$$`, `$$a+b$$` — чистый LaTeX, без вычислений.

**Таблицы**: ячейки `$$u/2$$`, `$$u=$$` вычисляются; `$$!…$$` — чистый LaTeX;
разделитель `|:--:|` задаёт выравнивание колонки; таблица центрирована.

**Списки**: `- ` и `1. `, вложенность отступом 2 пробела, до 3 уровней.

**Картинки**: `![подпись](файл.png)` — файл в images/ проекта (внутри ZIP).
Вставка: «Вставка → Картинка…», drag&drop файла, Ctrl+V со скриншотом.

**Оглавление**: `[toc]` — точки и номера страниц как в Word, ссылки кликабельны.

**Заголовки**: `#`/`##`/`###`; локально `# Титл {center,12pt}` (или `{align:center,size:12pt}`).