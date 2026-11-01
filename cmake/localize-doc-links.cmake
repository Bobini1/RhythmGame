cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED DOCS_HTML_DIRECTORY)
    message(FATAL_ERROR "DOCS_HTML_DIRECTORY must be defined")
endif()

# Markdown uses published URLs so its links also work on GitHub. In generated
# HTML, keep those links within the current copy of the documentation.
set(public_docs_url "https://bobini1.github.io/RhythmGame/")
file(GLOB_RECURSE html_pages "${DOCS_HTML_DIRECTORY}/*.html")
foreach(html_page IN LISTS html_pages)
    file(READ "${html_page}" html_contents)
    string(FIND "${html_contents}" "href=\"${public_docs_url}" link_index)
    if(link_index EQUAL -1)
        continue()
    endif()

    get_filename_component(page_directory "${html_page}" DIRECTORY)
    file(RELATIVE_PATH docs_root "${page_directory}" "${DOCS_HTML_DIRECTORY}")
    string(REGEX REPLACE "/+$" "" docs_root "${docs_root}")
    if(docs_root STREQUAL "")
        set(docs_root ".")
    endif()

    # A homepage link must name index.html when opened through file:// too.
    foreach(suffix IN ITEMS "\"" "#" "?")
        string(REPLACE "href=\"${public_docs_url}${suffix}"
            "href=\"${docs_root}/index.html${suffix}"
            html_contents "${html_contents}")
    endforeach()
    string(REPLACE "href=\"${public_docs_url}" "href=\"${docs_root}/"
        html_contents "${html_contents}")
    file(WRITE "${html_page}" "${html_contents}")
endforeach()
