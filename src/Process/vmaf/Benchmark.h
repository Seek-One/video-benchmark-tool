#ifndef VMAF_BENCHMARK_H_
#define VMAF_BENCHMARK_H_

#include <vector>

#include <QObject>
#include <QStringList>
#include <QString>
#include <QThread>

#include "Experiment.h"

namespace avcodec {
	struct EncoderParameters;
}

namespace vmaf {
	class Benchmark: public QObject {
	Q_OBJECT

	public:
		void start(const QString& szVideoFileName, int iMinCRF, int iMaxCRF, const QStringList& listPreset);

	private slots:
		void handleBenchmarkFinished();
	};

	class BenchmarkThread : public QThread {
	Q_OBJECT

	public:
		BenchmarkThread(const QString& szVideoFileName, int iMinCRF, int iMaxCRF, const QStringList& listPreset);

	signals:
		void benchmarkFinished();

	private:
		virtual void run() override;

		bool decodeOriginalVideoFile(const QString& szVideoFileName, QVector<QByteArray>& yuvFrames);
		void runExperiments(const QVector<QByteArray>& yuvFrames);

	private:
		QString m_szVideoFileName;
		int m_iMinCRF;
		int m_iMaxCRF;
		QStringList m_listPreset;

		avcodec::EncoderParameters m_originalCodecParameters;
		// Need to use STL vector to have emplace_back method
		std::vector<ExperimentThread> m_poolThreads;
	};
}

#endif // BENCHMARK_VMAF_H_