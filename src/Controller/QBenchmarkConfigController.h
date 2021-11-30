#ifndef Q_BENCHMARK_CONFIG_CONTROLLER_H_
#define Q_BENCHMARK_CONFIG_CONTROLLER_H_

#include <QObject>

#include "Process/BenchmarkVMAF.h"

namespace view {
	class QMainView;
	class QBenchmarkConfigView;
}

namespace ctrl {
	class QBenchmarkConfigController: public QObject {
	Q_OBJECT

	public:
		QBenchmarkConfigController(view::QMainView& mainView);

	private slots:
		void onBrowseFile();
		void onStartBenchmark();

	private:
		view::QBenchmarkConfigView& m_benchmarkConfigView;
		proc::BenchmarkVMAF m_benchmarkVMAF;
	};
}

#endif // Q_BENCHMARK_CONFIG_CONTROLLER_H_