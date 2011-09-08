

#ifndef __voKEGGImageView_h
#define __voKEGGImageView_h

// Qt includes
#include <QScopedPointer>

// Visomics includes
#include "voView.h"

class voKEGGImageViewPrivate;

class voKEGGImageView : public voView
{
  Q_OBJECT
public:
  typedef voView Superclass;
  voKEGGImageView(QWidget * newParent = 0);
  virtual ~voKEGGImageView();

  virtual void setDataObject(voDataObject* dataObject);

protected:
  void setupUi(QLayout * layout);

protected:
  QScopedPointer<voKEGGImageViewPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(voKEGGImageView);
  Q_DISABLE_COPY(voKEGGImageView);
};

#endif