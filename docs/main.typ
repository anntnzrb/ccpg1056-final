#import "@preview/sourcerer:0.2.1": code
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

// https://www.mermaidchart.com/app/projects/bed189c5-7739-4bfb-a9ad-8aadd6fc612d/diagrams/fc9137a0-e48e-4a9b-a93b-2b27f80860a7/version/v0.1/edit
#figure(
  image("assets/uml.png", width: 55%), caption: "Diagrama de Actividades del Pipeline",
)

#pagebreak()

= Limitaciones y Resoluciónes
Durante el desarrollo del proyecto se presentaron aalgunas limitaciones, cada
una de las cuales requirió soluciones específicas para garantizar que el
programa funcione como fue originalmente intencionado.

== A

#lorem(10)

== B

#lorem(10)

== C

#lorem(10)

#pagebreak()

= Salidas de Pantalla y Ejecución

#lorem(10)

#pagebreak()

= Anexos

#lorem(10)

#pagebreak()

#bibliography("bib.bib", style: "ieee", full: true, title: "Referencias")