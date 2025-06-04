# File modified by https://github.com/dhanraj-s

add_executable(xapp_kpm_rc 
  xapp_kpm_rc.c
  ../../../../src/util/alg_ds/alg/defer.c
  )
 
target_link_libraries(xapp_kpm_rc
                      PUBLIC
                      e42_xapp
                      -pthread
                      -lsctp
                      -ldl
                      )

add_executable(xapp_scheduler
  xapp_scheduler.c
  ../../../../src/util/alg_ds/alg/defer.c
  )
 
target_link_libraries(xapp_scheduler
                      PUBLIC
                      e42_xapp
                      -pthread
                      -lsctp
                      -ldl
                      )


