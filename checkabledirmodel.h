#ifndef FPSUBMIT_CHECKABLEDIRMODEL_H_
#define FPSUBMIT_CHECKABLEDIRMODEL_H_

#include <QFileSystemModel>
#include <QSet>
#include <QDebug>

class CheckableDirModel : public QFileSystemModel
{
	Q_OBJECT

public:
	Qt::ItemFlags flags(const QModelIndex& index) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

	QList<QString> selectedDirectories();

private:
	Qt::CheckState findParentState(const QModelIndex &index) const;
	void updateParents(const QModelIndex &index);
	void updateVisibleChildren(const QModelIndex &parent, Qt::CheckState);

	bool hasIndexState(const QModelIndex &index) const
	{
		return m_state.contains(filePath(index));
	}

	void setIndexState(const QModelIndex &index, Qt::CheckState state) const
	{
		//qDebug() << "Setting state for " << filePath(index) << " to " << state;
		m_state.insert(filePath(index), state);
	}

	Qt::CheckState getIndexState(const QModelIndex &index) const
	{
		return m_state.value(filePath(index), Qt::Unchecked);
	}

	mutable QMap<QString, Qt::CheckState> m_state;
};

#endif
