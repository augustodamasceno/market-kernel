# Code Generation
**Goal:** Generate high-performance, idempotent, and strictly formatted code.

## 1. Author & License Defaults
*(Pre-filled — update if needed before applying to any template below.)*

- **Year:** Current Year (e.g., 2026)
- **Author:** Augusto Damasceno
- **Email:** augustodamasceno@protonmail.com
- **Open Source License:** BSD-2-Clause
- **Open Source License Name:** BSD 2-Clause License
- **Open Source License Badge URL:** `https://img.shields.io/badge/License-BSD_2--Clause-orange.svg`
- **Closed Source License Identifier:** LicenseRef-`<BUSINESS>`-Proprietary
- **Project Link:** `https://github.com/augustodamasceno/`

---

## 2. Global Standards and Doc Templates

### Copyright Headers
**Rule:** Insert the following header at the very top of every new file using values from **Section 1**.

**Template (Open Source):**
```text
/*
 * <FILE DESCRIPTION>
 *
 * Copyright (c) <YEAR>, <AUTHOR>.
 * All rights reserved.
 *
 * SPDX-License-Identifier: <OPEN SOURCE LICENSE>
 */
```

**Template (Closed Source):**
```text
/*
 * <FILE DESCRIPTION>
 *
 * Copyright (c) <YEAR>, <BUSINESS>.
 * All rights reserved.
 *
 * SPDX-License-Identifier: <CLOSED SOURCE LICENSE IDENTIFIER>
 * This software is proprietary and confidential.
 */
```

### Indentation & Formatting
- **Indentation:** STRICTLY 4 spaces. No tabs.
- **Idempotency:** Ensure functions are pure where possible. Retries must not duplicate side effects. Handle errors gracefully.

### Use this template for main READMEs
*(Use values from **Section 1** for all `<FIELD>` placeholders.)*

```md
<div align="center">

# **<PROJECT-NAME>**

*<PROJECT-DESCRIPTION>*

<p>
  <a href="#"><img alt="License" src="<OPEN SOURCE LICENSE BADGE URL>"></a>
</p>

</div>

---

## **Contact**

* **Email:** [<EMAIL>](mailto:<EMAIL>)

---

## **License**

This project is licensed under <OPEN SOURCE LICENSE NAME>.

<small>Copyright &copy; <YEAR>, <AUTHOR>. All rights reserved.</small>

---

```

### Use this template for Jupyter
*(Use values from **Section 1** for all `<FIELD>` placeholders.)*

```md
<center><h1><TITLE></h1></center>
<center><i><DESCRIPTION></i></center>

---

**Author:** <AUTHOR>
**Project Link:** [<PROJECT LINK>](<PROJECT LINK>)
**Last Updated:** <DATE>

---

<div style="text-align: right;">
<small>Copyright © <YEAR>, <AUTHOR></small><br>
<small>SPDX-License-Identifier: <code><OPEN SOURCE LICENSE></code></small>
</div>
```
---

## 3. Output Format: Markdown

- **Structure:** Use Headings (`#`) for hierarchy.
- **Lists:** Use Bullet points (`-`) for rules.
- **Tables:** Use for style conventions or data comparison.
- **Code:** Always wrap code in triple backticks with language tags (e.g., `cpp`).
- **Quotes:** Do NOT use blockquotes (`>`) unless quoting a specific text source.

