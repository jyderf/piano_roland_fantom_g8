# Aurea Piano

Instrumento de piano por **modelado modal matemático** para Windows, con salida
VST3 y aplicación independiente. La síntesis genera tres cuerdas ligeramente
desafinadas por nota, parciales inarmónicos, decaimiento, contacto de martillo y
resonancia de caja. No carga muestras ni utiliza material propietario de Roland
o Modartt. Es un piano original inspirado en el carácter brillante de los
pianos de escenario; **no es una emulación medida ni una copia exacta del
Fantom G8 o Pianoteq**.

**Estado actual:** este repositorio contiene código fuente y scripts para
desarrolladores. Todavía no incluye un instalador `.exe` listo para usuarios ni
un plugin compilado y firmado. Quien use la futura versión final debería poder
instalarla y tocar sin instalar CMake, Visual Studio ni herramientas de firma.

El proyecto del instalador está en `Installer/AureaPiano.iss`. Cuando existan
binarios comprobados, empaqueta la aplicación independiente y el VST3 en un
único `.exe` con desinstalador. Puede incluir AAX solo después de su firma.

## Probar ahora con doble clic

Abre `demo/AureaPiano-demo.html` con **Chrome o Edge** y pulsa **Activar audio**.
Toca las teclas dibujadas con el ratón o usa `A W S E D F T G Y H U J K O L P`.
La demo permite probar capas, split, bajo, EQ, compresor, delay, reverb y
sustain. Es una implementación local para escuchar la idea del piano; **no es
el ejecutable Standalone ni el VST3/AAX final**, y no sirve para medir su
latencia o calidad definitiva. El MIDI del navegador puede estar bloqueado al
abrir un archivo local; el ratón y el teclado del computador no lo necesitan.
Pulsa **Probar seco / sin pedal** para anular temporalmente el sustain, EQ,
compresor, delay y reverb y oír una liberación breve de la nota. Pulsa de nuevo
para recuperar los controles anteriores. El control **Seco** por sí solo
ajusta la señal directa y puede mezclarse con efectos.

## Funciones

- Dos capas: piano o bajo en la capa dual.
- Cada capa cubre todo el teclado, solo la izquierda, solo la derecha, o queda
  apagada. La nota de división se puede fijar entre 0 y 127.
- Transposición independiente de −24 a +24 semitonos (−2 a +2 octavas),
  afinación fina y curva de velocidad por capa.
- Volumen, brillo, ataque, liberación, dureza del martillo, resonancia y
  anchura estéreo por capa.
- Sustain mediante pedal MIDI CC64 o interruptor en pantalla; pedal suave CC67.
- EQ paramétrico de 4 bandas con presets; compresor con umbral, relación,
  ataque, liberación, ganancia y presets.
- Señal seca, delay estéreo, reverb, volumen maestro y limitación suave.
- Teclado en pantalla con ratón y teclado del computador; MIDI estándar desde
  un DAW o desde la configuración Audio/MIDI de la aplicación independiente.
- Parámetros automatizables y estado guardado con el proyecto del DAW.

## Compatibilidad con programas de audio

La versión prevista del plugin es **VST3 de 64 bits**. Podrá cargarse en un DAW
para Windows que admita instrumentos VST3, una vez compilado y probado allí.
**Pro Tools 12 no carga VST3 directamente**: requiere un instrumento **AAX
Native de 64 bits**. El proyecto puede generar opcionalmente un paquete AAX,
pero **sin firma no cargará en una instalación normal de Pro Tools 12**. La
aplicación independiente se puede abrir aparte, pero eso no equivale a un
plugin integrado en Pro Tools.

## Compilación en Windows para desarrolladores

### Compilar en GitHub sin instalar herramientas de desarrollo

El flujo [`.github/workflows/build-windows.yml`](.github/workflows/build-windows.yml)
usa un equipo Windows de GitHub Actions con Visual Studio 2026, CMake e Inno
Setup. En el repositorio de GitHub, abre **Actions > Compilar Aurea Piano para
Windows > Run workflow**. Si la compilación y las pruebas terminan bien, descarga
el archivo `AureaPiano-0.1.0-win64-setup` desde **Artifacts**; dentro estará el
instalador `.exe`. Los artefactos se conservan 30 días y pueden requerir iniciar
sesión en GitHub. Si se publica la etiqueta `v0.1.0`, el flujo también crea una
versión preliminar en **Releases** con descarga directa del instalador. El
instalador contiene Standalone y VST3; no incluye AAX.

El flujo está preparado, pero aún no se ha ejecutado en GitHub ni se ha
verificado el resultado de compilación en Windows.

### Compilar en tu propio Windows

Requiere **Visual Studio Community 2026 con Desktop development with C++**,
**CMake 4.2 o superior** (agregado a `PATH`), **Git** y acceso a internet para
la primera descarga de JUCE 9.0.6. Desde
PowerShell, en esta carpeta:

```powershell
.\install-windows.ps1 -Launch
```

El script configura el proyecto, compila VST3 y Standalone, ejecuta la prueba
del motor e instala ambos para el usuario actual. `-Launch` abre inmediatamente
la aplicación independiente. El VST3 queda en
`%LOCALAPPDATA%\Programs\Common\VST3\Aurea Piano.vst3` y la aplicación en
`%LOCALAPPDATA%\Programs\Aurea Piano\Aurea Piano.exe`. Vuelve a analizar los
plugins en tu DAW. Si ya compilaste el proyecto, usa
`.\install-windows.ps1 -SkipBuild -Launch`.

También puedes ejecutar solo `.\build-windows.ps1`; los productos aparecerán
en `build/AureaPiano_artefacts/Release/`.

El generador predeterminado es Visual Studio 2026. Si ya tienes Visual Studio
2022 con las herramientas de C++, usa
`.\install-windows.ps1 -Launch -Generator "Visual Studio 17 2022"` y CMake
3.22 o superior. Si cambias de versión de Visual Studio, configura en un
directorio de compilación nuevo para evitar el caché de CMake anterior.

### Para Pro Tools 12 (AAX)

1. Con las herramientas de compilación anteriores, ejecuta
   `.\build-windows.ps1 -WithAAX`. JUCE 9.0.6 incluye el SDK AAX necesario para
   generar el paquete `Aurea Piano.aaxplugin` en
   `build/AureaPiano_artefacts/Release/AAX/`.
2. El AAX generado está **sin firmar**. Para que Pro Tools comercial lo cargue,
   hay que seguir el programa de desarrollo de Avid y firmar el paquete con las
   herramientas de PACE. El SDK de Avid y su acuerdo están disponibles en
   <https://developer.avid.com/aax/>. Esto requiere cuentas de Avid e iLok;
   la comercialización requiere además las herramientas y licencia de firma.
3. Tras firmarlo, se instala en
   `C:\Program Files\Common Files\Avid\Audio\Plug-Ins` y se prueba en una copia
   real de Pro Tools 12. La compatibilidad con esa versión antigua no está
   verificada. El instalador VST3 de este proyecto no instala AAX.

Como alternativa para una prueba en Pro Tools 12, un anfitrión **AAX de
instrumentos** que cargue VST3 puede abrir el Aurea Piano VST3 dentro de Pro
Tools. Blue Cat documenta este uso con **PatchWork Synth**; es software externo
y conviene probar su versión de evaluación con Pro Tools 12 antes de comprarlo.

### Crear un instalador de distribución

Esta sección es **para quien fabrica el producto**, no para el usuario final.
Tras compilar y probar `Standalone` y `VST3`, instala Inno Setup 7 y ejecuta
`.\build-installer.ps1`. Se generará
`dist/AureaPiano-0.1.0-win64-setup.exe`, que instala la aplicación y,
opcionalmente, el VST3 para el usuario actual sin permisos de administrador.
El usuario final solo necesita ejecutar ese `.exe`.

Para incluir AAX, firma primero el paquete mediante el proceso de Avid/PACE y
colócalo en `release/signed/Aurea Piano.aaxplugin`. Después ejecuta
`.\build-installer.ps1 -WithAAX`. Ese instalador requiere permisos de
administrador porque AAX debe instalarse en la carpeta común de Avid. El
script comprueba que el paquete exista, pero **no verifica criptográficamente
la firma**; eso debe validarse en Pro Tools 12 antes de distribuirlo.

JUCE tiene sus propias condiciones de licencia; revísalas antes de distribuir
binarios comerciales.

## Uso rápido

1. En un DAW, crea una pista de instrumento con **Aurea Piano** y envíale MIDI.
   En la aplicación independiente abre **Opciones > Audio/MIDI**, selecciona
   una salida de audio de baja latencia y habilita tu entrada MIDI.
2. Para bajo a la izquierda y piano a la derecha: `Zona principal = Derecha`,
   `Zona dual = Izquierda`, `Timbre dual = Bajo`, y ajusta `División MIDI`.
3. Para dos pianos en toda la extensión: pon ambas zonas en `Todo` y el timbre
   dual en `Piano`. Puedes transponer y equilibrar cada capa por separado.
4. Prueba notas con el ratón o las teclas `A W S E D F T G Y H U J K O L P`.
   Si el teclado del computador no responde tras mover un control, haz clic en
   el teclado dibujado para devolverle el foco. Los botones `− oct` y `+ oct`
   cambian la octava de estas teclas.

Un Yamaha PSR-E421, P-115, Korg Kross 2 u otro equipo puede usarse si Windows
lo reconoce como dispositivo MIDI. La conexión y el controlador USB dependen
del equipo; en VST3, la selección de entradas se hace en el DAW. La latencia
percibida depende también del controlador de audio y del tamaño del búfer. El
motor no añade latencia de anticipación.
