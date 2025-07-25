# Black Book
This is a music player for PC based on SQLite, designed for easy and powerful searching. 

<img width="2559" height="1365" alt="2025-07-24_21-45" src="https://github.com/user-attachments/assets/3dbe9335-986c-4be4-9d86-904d812a12a2" />

## Keybinds
| Key               | Effect                            |
| ----------------- | --------------------------------- |
| K OR Shift+P      | Previous song                     |
| J OR Shift+N      | Next song                         |
| H OR Left Arrow   | Back 5 seconds                    |
| L OR Right Arrow  | Forward 5 seconds                 |
| Space             | Pause/play                        |
| Ctrl+T            | Open new search menu              |
| F5 OR Crl+R       | Reload from database              |
| Ctrl+I            | Import files menu                 |

## Tags & Inheritance
Tags are the main way to filter songs. Usually these are simple things like the artist or album name, but can also be genres or any arbitrary text.
Tags can *inherit* from each other, so that one tag automatically adds another. For example `artist:frequency` may be a child of `drum and bass`,
which is a child of `electronic`. Now by adding the `artist:frequency` tag (which can be automatically scraped from the MP3 metadata), the song will
show up in searches for all 3 tags. In the photo above, you can see that `100 gecs` songs appear in searches for `electronic` because
`artist:100 gecs` is a child of `hyperpop` which is a child of `electronic`. These recursive parents are supported up to 3 layers (hardcoded but
could be extended).

Tags can also be used to create persistent playlists (for example you could import a bunch of songs you got from a friend and then use their name as
a tag, and get a "music from Joe" playlist).

## Searching
The database will give autocomplete suggestions for tags as you type. The controls are the same as an IDE autocomplete (up/down keys to select a
suggestion). When you search for multiple tags at once, it's treated as an `AND` (all the tags must be present to show in the results).
In the future, I want to add
a "smart" expression parser that can handle complex searches like `(artist:nas OR artist:jurassic 5) AND (year:1990s OR year:1980s) AND NOT year:1992`.
SQLite can already do this easily, the problem is the parsing and UI code.

## Internals
Most of the core behaviour is in a `blackbook_core` class independent of any ImGui code. The idea is that eventually, it could be ported to a new GUI
library (e.g. a web or mobile frontend), without breaking the ImGui app or duplicating code. This also makes a CLI frontend much easier, since it forces
there to be a usable headless API. At the moment, thumbnail handling is coupled to ImGui code since it deals directly with OpenGL.
