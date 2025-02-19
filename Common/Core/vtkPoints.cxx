/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPoints.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkPoints* vtkPoints::New(int dataType)
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPoints");
  if (ret)
  {
    if (dataType != VTK_FLOAT)
    {
      static_cast<vtkPoints*>(ret)->SetDataType(dataType);
    }
    return static_cast<vtkPoints*>(ret);
  }
  // If the factory was unable to create the object, then create it here.
  vtkPoints* result = new vtkPoints(dataType);
  result->InitializeObjectBase();
  return result;
}

//------------------------------------------------------------------------------
vtkPoints* vtkPoints::New()
{
  return vtkPoints::New(VTK_FLOAT);
}

//------------------------------------------------------------------------------
// Construct object with an initial data array of type float.
vtkPoints::vtkPoints(int dataType)
{
  this->Data = vtkFloatArray::New();
  this->Data->Register(this);
  this->Data->Delete();
  this->SetDataType(dataType);

  this->Data->SetNumberOfComponents(3);
  this->Data->SetName("Points");

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
}

//------------------------------------------------------------------------------
vtkPoints::~vtkPoints()
{
  this->Data->UnRegister(this);
}

//-----------------GetTuples (id list)------------------------------------------
// Copy from vtkDataArray with the only difference that we know the tuple size
struct GetTuplesFromListWorker
{
  vtkIdList* Ids;

  GetTuplesFromListWorker(vtkIdList* ids)
    : Ids(ids)
  {
  }

  template <typename Array1T, typename Array2T>
  void operator()(Array1T* src, Array2T* dst) const
  {
    const auto srcTuples = vtk::DataArrayTupleRange<3>(src);
    auto dstTuples = vtk::DataArrayTupleRange<3>(dst);

    vtkIdType* srcTupleId = this->Ids->GetPointer(0);
    vtkIdType* srcTupleIdEnd = this->Ids->GetPointer(this->Ids->GetNumberOfIds());

    auto dstTupleIter = dstTuples.begin();
    while (srcTupleId != srcTupleIdEnd)
    {
      *dstTupleIter++ = srcTuples[*srcTupleId++];
    }
  }
};

//------------------------------------------------------------------------------
// Given a list of pt ids, return an array of points.
void vtkPoints::GetPoints(vtkIdList* ptIds, vtkPoints* outPoints)
{
  outPoints->Data->SetNumberOfTuples(ptIds->GetNumberOfIds());

  // We will NOT use this->Data->GetTuples() for 4 reasons:
  // 1) It does a check if the outPoints->Data array is a vtkDataArray, which we now it is.
  // 2) It does a check if the number of components is the same, which we know it's 3 for both.
  // 3) It has a really expensive Dispatch2::Execute, and every time it tries many array types.
  //    Points are 99% of the times floats or doubles, so we can avoid A LOT of failed FastDownCast
  //    operations, by utilizing this knowledge.
  // 4) The Worker isn't aware of the number of components of the tuple which slows down the access.
  using Dispatcher =
    vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
  GetTuplesFromListWorker worker(ptIds);
  if (!Dispatcher::Execute(this->Data, outPoints->Data, worker))
  {
    // Use fallback if dispatch fails.
    worker(this->Data, outPoints->Data);
  }
}

//------------------------------------------------------------------------------
// Determine (xmin,xmax, ymin,ymax, zmin,zmax) bounds of points.
void vtkPoints::ComputeBounds()
{
  if (this->GetMTime() > this->ComputeTime)
  {
    this->Data->ComputeScalarRange(this->Bounds);
    this->ComputeTime.Modified();
  }
}

//------------------------------------------------------------------------------
// Return the bounds of the points.
double* vtkPoints::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

//------------------------------------------------------------------------------
// Return the bounds of the points.
void vtkPoints::GetBounds(double bounds[6])
{
  this->ComputeBounds();
  memcpy(bounds, this->Bounds, 6 * sizeof(double));
}

//------------------------------------------------------------------------------
vtkMTimeType vtkPoints::GetMTime()
{
  vtkMTimeType doTime = this->Superclass::GetMTime();
  if (this->Data->GetMTime() > doTime)
  {
    doTime = this->Data->GetMTime();
  }
  return doTime;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkPoints::Allocate(vtkIdType sz, vtkIdType ext)
{
  int numComp = this->Data->GetNumberOfComponents();
  return this->Data->Allocate(sz * numComp, ext * numComp);
}

//------------------------------------------------------------------------------
void vtkPoints::Initialize()
{
  this->Data->Initialize();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPoints::Modified()
{
  this->Superclass::Modified();
  if (this->Data)
  {
    this->Data->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkPoints::GetDataType() const
{
  return this->Data->GetDataType();
}

//------------------------------------------------------------------------------
// Specify the underlying data type of the object.
void vtkPoints::SetDataType(int dataType)
{
  if (dataType == this->Data->GetDataType())
  {
    return;
  }

  this->Data->Delete();
  this->Data = vtkDataArray::CreateDataArray(dataType);
  this->Data->SetNumberOfComponents(3);
  this->Data->SetName("Points");
  this->Modified();
}

//------------------------------------------------------------------------------
// Set the data for this object. The tuple dimension must be consistent with
// the object.
void vtkPoints::SetData(vtkDataArray* data)
{
  if (data != this->Data && data != nullptr)
  {
    if (data->GetNumberOfComponents() != this->Data->GetNumberOfComponents())
    {
      vtkErrorMacro(<< "Number of components is different...can't set data");
      return;
    }
    this->Data->UnRegister(this);
    this->Data = data;
    this->Data->Register(this);
    if (!this->Data->GetName())
    {
      this->Data->SetName("Points");
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Deep copy of data. Checks consistency to make sure this operation
// makes sense.
void vtkPoints::DeepCopy(vtkPoints* da)
{
  if (da == nullptr)
  {
    return;
  }
  if (da->Data != this->Data && da->Data != nullptr)
  {
    if (da->Data->GetNumberOfComponents() != this->Data->GetNumberOfComponents())
    {
      vtkErrorMacro(<< "Number of components is different...can't copy");
      return;
    }
    this->Data->DeepCopy(da->Data);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Shallow copy of data (i.e. via reference counting). Checks
// consistency to make sure this operation makes sense.
void vtkPoints::ShallowCopy(vtkPoints* da)
{
  this->SetData(da->GetData());
}

//------------------------------------------------------------------------------
unsigned long vtkPoints::GetActualMemorySize()
{
  return this->Data->GetActualMemorySize();
}

//------------------------------------------------------------------------------
void vtkPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Data: " << this->Data << "\n";
  os << indent << "Data Array Name: ";
  if (this->Data->GetName())
  {
    os << this->Data->GetName() << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  const double* bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";
}
