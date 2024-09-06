#import "@preview/codly:1.0.0": *
#import "template.typ": *

#show: project.with(
  title: "PA4: Diseño de un Pipeline con IPC", authors: (
    (
      name: "Juan Antonio González", email: "juangonz@espol.edu.ec", affiliation: "ESPOL (FIEC)",
    ), (
      name: "Oscar García Tinoco", email: "omgarcia@espol.edu.ec", affiliation: "ESPOL (FIEC)",
    ),
  ),
)

#show link: it => [#set text(blue); #underline(it)]

#show: codly-init.with()
#codly(
  languages: (
    sh: (name: "sh", icon: "\u{f192} ", color: rgb("#CE412B")), c: (name: "C", icon: "\u{f13a} ", color: rgb("#555555")),
  ), number-format: none,
)

= Problemática a Resolver

El objetivo principal de este proyecto es aplicar técnicas de sincronización y
comunicación entre procesos que han sido revisadas en clase. Esta solución debe
implementar un pipeline de procesamiento de imágenes utilizando comunicación
entre procesos (*IPC*) y técnicas de sincronización en *C*.

La problemática específica consiste en diseñar e implementar un sistema
compuesto por *tres programas principales* que trabajen de forma concurrente:

1. *Publicador*: lee una imagen de entrada en formato _BMP_ y la almacena en un
  recurso de memoria compartida

2. *Desenfocador*: aplica un filtro de desenfoque (*blur*) a la primera mitad de la
  imagen

3. *Realzador*: aplica un kernel de realce de bordes (*edge detection*) a la
  segunda mitad de la imagen

Estos programas deben sincronizarse adecuadamente para procesar la imagen en
etapas, garantizando que:

- El *desenfocador* y el *realzador* se ejecuten como _procesos independientes_ una
  vez que el *publicador* haya cargado la imagen en memoria compartida
- Al finalizar, se combinen las dos porciones procesadas de la imagen en una sola
  y se guarde en disco

A continuación se presenta un diagrama de actividades que describe el
comportamiento esperado del pipeline @mermaid-chart:

// https://www.mermaidchart.com/app/projects/bed189c5-7739-4bfb-a9ad-8aadd6fc612d/diagrams/fc9137a0-e48e-4a9b-a93b-2b27f80860a7/version/v0.1/edit
#figure(
  image("assets/uml.png", width: 80%), caption: "Diagrama de Actividades del Pipeline",
)

#pagebreak()

= Filtros e Implementación

== Filtro de Desenfoque (Blur)

Para este filtro se decidió emplear una versión simplificada del _Filtro Gaussiano_ (*Gaussian
Blur*) con un kernel `3x3`. Este filtro es eficaz para suavizar las imágenes,
reducir el ruido y eliminar detalles finos, lo que lo convierte en una
herramienta esencial para el procesamiento de imágenes @wiki:Gaussian_blur.

El kernel utilizado para este filtro es:

$ mat(1, 2, 1;2, 4, 2;1, 2, 1) $

== Filtro de Realce (Edge Detection)

Para el filtro de realce de bordes (*edge detection*), se implementó un _Filtro Laplaciano_ utilizando
un kernel `3x3`. Este filtro es conocido por su capacidad para detectar bordes
de manera isotrópica, lo que lo hace ideal para resaltar detalles finos en la
imagen @sbme_cv_notes_2018.

El kernel utilizado para este filtro es:

$ mat(-1, -1, -1;-1, 8, -1;-1, -1, -1) $

#pagebreak()

= Limitaciones y Resoluciónes
Durante el desarrollo del proyecto se presentaron algunas limitaciones, cada una
de las cuales requirió soluciones específicas para garantizar que el programa
funcione como fue originalmente intencionado.

== Sincronización Entre Procesos

El principal problema surgió debido a que el publicador continuaba su ejecución
sin haber confirmado que los filtros que se ejecutaron como procesos
independientes, terminaran de ejecutarse. Estos ocasionaba que el publicador
intentara continuar con su ejecución y guardar una imagen incompleta o corrupta,
lo que resultaba en que el programa finalizara abruptamente o crasheara.

La solución a este problema fue implementar un publicador que ejecutara ambos
filtros de manera simultánea una vez que la memoria compartida ya se haya
mapeado, y que espere la finalización de estos procesos mediante la función `waitpid` para
garantizar que no se continue con la ejecución utilizando una imagen incompleta
y posiblemente dañada.

```c
// publicador.c
// ...
filtro_realzador =
    apply_filter(image, num_threads, "realzador", 0, mid_row);
filtro_desenfocador = apply_filter(image, num_threads, "desenfocador",
                                   mid_row, image->norm_height);

// esperar a que los filtros terminen
int status;
if (filtro_desenfocador == -1 || filtro_realzador == -1 ||
    waitpid(filtro_desenfocador, &status, 0) == -1 || !WIFEXITED(status) ||
    WEXITSTATUS(status) != 0 ||
    waitpid(filtro_realzador, &status, 0) == -1 || !WIFEXITED(status) ||
    WEXITSTATUS(status) != 0) {
    fprintf(stderr, "Error procesando la imagen\n");
    fclose(source);
    return 1;
}
// ...
```

== División de Responsabilidades Entre Filtros

Otro problema que se presentó fue garantizar que cada filtro procesara
exactamente la mitad de la imagen asignada. Esto implicaba evitar la
sobreescritura de píxeles o la omisión de los que ya habían sido tratados por el
otro filtro, lo cual resultaba en la generación de una imagen con un
procesamiento de baja calidad o impreciso.

Para resolver este problema se implementó un publicador que asignaba los rangos
de filas y parámetros que definían el área de operación para cada filtro. Esta
asignación detallada permitía que cada filtro procesara exactamente la mitad de
la imagen que le correspondía. Una vez establecidos estos parámetros se logró la
división del trabajo mediante el uso de múltiples hilos, mejorando el
rendimiento del programa y acotando las instrucciones solicitadas.

#pagebreak()

== Gestión de Memoria

La gestión de memoria se presentó como un tópico bastante tedioso. Este problema
surgió debido a la necesidad de compartir datos entre procesos utilizando
memoria compartida, lo que aumentaba el riesgo de fugas de memoria si los
recursos no se liberaban adecuadamente.

Para lidiar con este problema, se utilizó memoria compartida mediante la
libreria *POSIX*. Se empleó la función `mmap` para mapear la memoria compartida.
Ahora, la asignación de memoria se realiza de manera dinámica según el tamaño de
la imagen la cual es liberada por la función `munmap` y `shm_unlink`.

```c
// publicador.c
int create_shm(const char *name, int *fd, void **pixels_image, size_t shm_size) {
    // crear el espacio de memoria compartida
    *fd = shm_open(name, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (*fd == -1 || ftruncate(*fd, shm_size) != 0) {
        return -1;
    }

    // mapear la memoria compartida
    *pixels_image =
        mmap(0, shm_size, PROT_WRITE | PROT_READ, MAP_SHARED, *fd, 0);

    // si no se pudo mapear, eliminar el espacio de memoria compartida
    if (*pixels_image == NULL) {
        shm_unlink(name);
        return -1;
    }

    return 0;
}
```

#pagebreak()

== Imágenes con Dimensiones Negativas

Algunas imágenes BMP tienen dimensiones negativas, las cuales tienen una
orientación invertida y requieren un procesamiento especial. Este problema
causaba que los filtros aplicaran sus efectos de manera incorrecta, e
inesperada. En consecuencia, se generaban imágenes distorsionadas o procesadas
erróneamente.

Se añadió un campo `is_bottom_up` a la estructura `BMP_Image` para identificar
estas imágenes, se ajustaron los cálculos de las filas de inicio y fin en la
función `apply_filter`. Esto permitió que los filtros procesen correctamente a
imágenes con dimensiones normales e invertidas, asegurando que los efectos se
apliquen en la dirección adecuada independientemente de la orientación de la
imagen.

```c
// bmp.h
typedef struct BMP_Image {
    BMP_Header header;
    int norm_height;
    int bytes_per_pixel;
    Pixel **pixels;
    int is_bottom_up; // identificar imgs al revés
} BMP_Image;
```

```c
// bmp.c
int read_bmp(FILE *fp, BMP_Image *img) {
    // ...

    // detectar si la img está al revés
    img->is_bottom_up = img->header.height_px > 0;
    img->norm_height = abs(img->header.height_px);

    // ...
}
```

#pagebreak()

= Salidas de Pantalla y Ejecución

#figure(
  image("assets/cap1.png", width: 100%), caption: "Compilación del programa",
)

#figure(
  image("assets/cap2.png", width: 100%), caption: "Ejecución del programa #1",
)

#figure(
  image("assets/cap3.png", width: 100%), caption: "Ejecución del programa #2",
)

#figure(
  image("assets/cap4.png", width: 100%), caption: "Ejecución de ejemplares de prueba",
)

#pagebreak()

= Anexos

== Repositorio Online

La totalidad del código desarrollado se encuentra disponible en el siguiente
#link("https://github.com/anntnzrb/ccpg1056-final")[repositorio de GitHub].

== Ejecución del Programa <exec-prog>

#quote[
  NOTA: Se recomienda referirse a la sección @docker-instructions para ejecutar el
  programa en un contenedor aislado usando Docker.
]

Primero se debe compilar el código utilizando el comando `make` en la terminal.
Solo por precaución, se sugiere ejecutar el comando `make clean` antes de
ejecutar
`make`, así se eliminan archivos residuales.

```sh
make clean && make
```

Posterior a esto, se crearán distintos ejecutables. El archivo que importa se
llama `pipeline` y es el encargado de ejecutar todo el pipeline.

Para ejecutar el programa, se puede, por ejemplo, hacerlo de la siguiente
manera:

```sh
./pipeline 4 outputs/result.bmp
```

Una vez que el programa esté ejecutándose, se solicita que se ingrese una ruta
de archivo para una imagen _BMP_ que se utilizará como entrada.

La consola muestra un mensaje como el siguiente:

```sh
Ingrese la ruta de la imagen (o 'q' para salir):
```

Se puede ingresar una ruta como la siguiente:

```sh
Ingrese la ruta de la imagen (o 'q' para salir): ./samples/test.bmp
```

En este caso, se ejecutará el programa con 4 hilos y la imagen resultante se
guardará en el archivo `outputs/result.bmp`.

#pagebreak()

=== Correr Ejemplos de Pruebas

Para correr los ejemplos de prueba, localizados en el directorio `samples/`, se
puede ejecutar el siguiente comando:

```sh
make clean && make samples
```

Los resultados se guardarán en el directorio `outputs/` con el mismo nombre de
la imagen de entrada más el sufijo `_sol`.

=== Ejecución del Programa con Docker <docker-instructions>

A pesar de que el programa es funcional, y está testeado para evitar problemas
de comportamientos inesperados, como leaks de memoria, entre otros. Se
recomienda ejecutar el programa utilizando
#link("https://www.docker.com/")[Docker]. Esto permite ejecutar el programa en
un contenedor aislado, evitando problemas en la máquina host.

La imagen configurada en el contenedor es muy pequeña, y no debería ocupar mucho
espacio en la máquina host.

Para ejecutar el programa con Docker, se puede utilizar el siguiente comando:

#quote[
  NOTA: Se debe ya tener instalado #link("https://docs.docker.com/get-docker/")[Docker]
  en la máquina host.
]

```sh
make docker-run
```

Por default, se corren los ejemplos de prueba provistos primero, y se ofrece una
consola interactiva para probar el programa con alguna imagen de entrada
proporcionada por el usuario, como se describe en la sección @exec-prog.

#pagebreak()

#bibliography("bib.bib", style: "ieee", full: true, title: "Referencias")