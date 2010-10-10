#include <QDebug>
#include "checkabledirmodel.h"

static const int CHECKABLE_COLUMN = 0;

Qt::ItemFlags CheckableDirModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = QFileSystemModel::flags(index);
	if (index.column() == CHECKABLE_COLUMN) {
		flags |= Qt::ItemIsUserCheckable;
	}
	return flags;
}

Qt::CheckState CheckableDirModel::findParentState(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return Qt::Unchecked;
	}
	if (hasIndexState(index)) {
		return getIndexState(index);
	}
	//qDebug() << "Looking up parent state for " << filePath(index);
	Qt::CheckState state = findParentState(index.parent());
	setIndexState(index, state);
	return state;
}

QVariant CheckableDirModel::data(const QModelIndex& index, int role) const
{
	if (index.isValid() && index.column() == CHECKABLE_COLUMN && role == Qt::CheckStateRole) {
		return findParentState(index);
	}
	return QFileSystemModel::data(index, role);
}

void CheckableDirModel::updateParents(const QModelIndex &parent)
{
	if (!parent.isValid()) {
		return;
	}

	int childRowCount = rowCount(parent);
	QSet<Qt::CheckState> childStates;
	for (int i = 0; i < childRowCount; i++) {
		childStates.insert(getIndexState(index(i, CHECKABLE_COLUMN, parent)));
	}

	if (childStates.size() > 1) {
		setIndexState(parent, Qt::PartiallyChecked);
		emit dataChanged(parent, parent);
	}
	else if (childStates.size() == 1) {
		setIndexState(parent, *childStates.begin());
		emit dataChanged(parent, parent);
	}

	updateParents(parent.parent());
}

void CheckableDirModel::updateVisibleChildren(const QModelIndex &parent, Qt::CheckState state)
{
	int childRowCount = rowCount(parent);
	for (int i = 0; i < childRowCount; i++) {
		const QModelIndex child = index(i, CHECKABLE_COLUMN, parent);
		if (hasIndexState(child)) {
			updateVisibleChildren(child, state);
		}
		setIndexState(child, state);
		emit dataChanged(child, child);
	}
}

bool CheckableDirModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (index.isValid() && index.column() == CHECKABLE_COLUMN && role == Qt::CheckStateRole) {
		Qt::CheckState state = Qt::CheckState(value.toInt()); 
		setIndexState(index, state);
		updateVisibleChildren(index, state);
		updateParents(index.parent());
		emit dataChanged(index, index);
		return true;
	}
	return QFileSystemModel::setData(index, value, role);
}

QList<QString> CheckableDirModel::selectedDirectories()
{
	return m_state.keys(Qt::Checked);
}

