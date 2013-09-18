PROGRAMS=janitor_test controlled_module_test controlled_module_ex_test simple_application_test

all: ${PROGRAMS}

janitor_test: janitor_test.cc janitor.h
	g++ -m32 -I/localdisk/tansongrong/VISServer/3rdparty/boost_1_46_1/include/ -L/localdisk/tansongrong/VISServer/3rdparty/boost_1_46_1/lib/boost/ -o janitor_test janitor_test.cc

controlled_module_test: controlled_module_test.cc controlled_module.h janitor.h
	g++ -m32 -I/localdisk/tansongrong/VISServer/3rdparty/boost_1_46_1/include/ -L/localdisk/tansongrong/VISServer/3rdparty/boost_1_46_1/lib/boost/ -lboost_system -lboost_thread -o controlled_module_test controlled_module_test.cc

controlled_module_ex_test: controlled_module_ex_test.cc controlled_module_ex.h controlled_module.h janitor.h
	g++ -m32 -I/localdisk/tansongrong/VISServer/3rdparty/boost_1_46_1/include/ -L/localdisk/tansongrong/VISServer/3rdparty/boost_1_46_1/lib/boost/ -lboost_system -lboost_thread -o controlled_module_ex_test controlled_module_ex_test.cc

simple_application_test: simple_application_test.cc simple_application.h controlled_module_ex.h controlled_module.h janitor.h
	g++ -m32 -I/localdisk/tansongrong/VISServer/3rdparty/boost_1_46_1/include/ -L/localdisk/tansongrong/VISServer/3rdparty/boost_1_46_1/lib/boost/ -lboost_system -lboost_thread -o simple_application_test simple_application_test.cc

clean:
	rm ${PROGRAMS}
