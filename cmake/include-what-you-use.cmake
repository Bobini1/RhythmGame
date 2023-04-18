# Generate compilation database
find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)

set(iwyu_command "${iwyu_path};-Xiwyu;--cxx17ns;-Xiwyu;--no_comments")
set_property(TARGET RhythmGame_lib PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_command})
set_property(TARGET RhythmGame_exe PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_command})
set_property(TARGET RhythmGame_test PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_command})