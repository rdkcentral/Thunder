All documentation for Thunder should be stored in the Thunder GitHub repository in the `docs` directory. Documentation is generated using [ReadTheDocs](https://readthedocs.org/) with the [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/) theme.

## How to update documentation

To work on documentation locally:

* Ensure you have Python 3 installed and working
* Install **mkdocs-material** from pip

```
$ pip install mkdocs-material
```

* From the root of this repository, run `mkdocs serve` to generate documentation and launch a local web server:

```
$ mkdocs serve
INFO     -  Building documentation...
INFO     -  Cleaning site directory
INFO     -  Documentation built in 0.45 seconds
INFO     -  [11:51:42] Watching paths for changes: 'docs', 'mkdocs.yml'
INFO     -  [11:51:42] Serving on http://127.0.0.1:8000/Thunder/
```

If everything is working, you should be able to access a local copy of the documentation at `http://localhost:8000/Thunder/`.

This site will automatically refresh as you edit the markdown files, making it easy to see your changes.

The `mkdocs.yml` file in the root of the repository defines the page hierarchy and layout. When adding new pages, update this file accordingly 

### GitHub Actions

A GitHub action is configured to automatically publish the latest version of the documentation in this repository to a GitHub pages site at [https://rdkcentral.github.io/Thunder/](https://rdkcentral.github.io/Thunder/)

The documentation website source code is on the `gh-pages` branch of this repo.

## Guidelines

Follow the below guidelines when writing documentation:

* Check all spelling/grammar before submitting changes
* Use admonitions where appropriate to call out import information in a document. See [here](https://squidfunk.github.io/mkdocs-material/reference/admonitions/) for all available options. 

```
!!! note
	Text you want to show up in the note box
```

!!! note
	Text you want to show up in the note box

* Surround all code samples with code blocks, ensuring the language is also set so syntax highlighting works correctly
* Refer to the Material for MkDocs reference for additional features: [https://squidfunk.github.io/mkdocs-material/reference/](https://squidfunk.github.io/mkdocs-material/reference/)
* Use [mermaid](https://mermaid.js.org/) diagrams where possible instead of embedding images
    * This helps ensure accessibility, loads faster and looks good on all screen sizes

````
``` mermaid
graph LR
  A[Start] --> B{Error?};
  B -->|Yes| C[Hmm...];
  C --> D[Debug];
  D --> B;
  B ---->|No| E[Yay!];
```
````

``` mermaid
graph LR
  A[Start] --> B{Error?};
  B -->|Yes| C[Hmm...];
  C --> D[Debug];
  D --> B;
  B ---->|No| E[Yay!];
```