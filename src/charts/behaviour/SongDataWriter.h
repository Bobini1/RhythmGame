//
// Created by bobini on 02.09.22.
//

#ifndef RHYTHMGAME_SONGDATAWRITER_H
#define RHYTHMGAME_SONGDATAWRITER_H
#include <string>

namespace charts::behaviour {
/**
 * @brief Writes song data to some destination.
 */
template<typename T, typename Var>
concept SongDataWriter =
  requires(T songDataWriter, const std::string& key, Var var) {
      /**
     * @brief Writes variable to some storage.
     * @param key Name of the variable.
     * @param var Value of the variable.
       */
      {
          songDataWriter.writeVar(key, var)
          } -> std::same_as<void>;
  };

} // namespace charts::behaviour
#endif // RHYTHMGAME_SONGDATAWRITER_H