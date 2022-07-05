install(
    TARGETS RhythmGame_exe
    RUNTIME COMPONENT RhythmGame_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
