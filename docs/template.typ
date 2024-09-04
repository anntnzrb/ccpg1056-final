#let project(
  title: "", authors: (), date: none, logo: "assets/espol_logo.png", body,
) = {
  // basic properties
  set document(author: authors.map(a => a.name), title: title)
  set page(paper: "us-letter", numbering: "1", number-align: end)
  set text(font: "Linux Libertine", lang: "es")

  // paragraphs
  show par: set block(above: 2em, below: 2em)

  set heading(numbering: "1.1")
  set par(leading: 0.75em)

  // title page
  v(0.6fr)
  if logo != none {
    image("assets/espol_logo.png", width: 90%)
  }

  v(15em)

  text(1.1em, date)
  v(1.2em, weak: true)
  text(2em, weight: 700, title)

  // authors
  pad(
    top: 0.7em, right: 20%, grid(
      columns: (1fr,) * calc.min(3, authors.len()), gutter: 1em, ..authors.map(author => align(start)[
        *#author.name* \
        #author.email \
        #author.affiliation
      ]),
    ),
  )

  v(2.4fr)
  pagebreak()

  // table of contents
  outline(depth: 3, indent: true)

  pagebreak()

  // main body
  set par(justify: true, leading: 1.5em)
  set text(hyphenate: false)

  body
}