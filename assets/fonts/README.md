# Chat fonts

The four unmodified Liberation Sans faces provide regular, bold, italic, and
bold italic chat text. They are distributed under the SIL Open Font License 1.1;
see LICENSE-Liberation.txt for the original attribution and complete license.

CMake copies this directory into the client's runtime `fonts/` directory and
installs it alongside the executable. Chat loads Latin, extended Latin, and
Cyrillic glyph ranges. Other unsupported glyphs use the font's replacement glyph;
the original UTF-8 text is retained when copying messages.
