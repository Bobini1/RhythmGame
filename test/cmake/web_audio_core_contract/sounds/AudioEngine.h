#pragma once

// The dependency audit only needs an independently compilable header at the
// forbidden production path. Qt's convenience macro would otherwise rewrite
// RealtimeMixer::emit in translation units where this file is force-included.
#ifdef emit
#undef emit
#endif
